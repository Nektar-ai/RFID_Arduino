#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <Wire.h>
#include "LiquidCrystal_PCF8574.h"
#include <string>
#include <sstream>

#define LED 15
#define PHO A0
#define DHTPIN 2
#define DHTTYPE DHT11
#define MOT 12

#define MQTT_PUB_TEMP "EPSI/DHT11/5C:CF:7F:B8:C4:75/TEMP"
#define MQTT_PUB_HUM "EPSI/DHT11/5C:CF:7F:B8:C4:75/HUM"
#define MQTT_PUB_LUM "EPSI/GL5516/5C:CF:7F:B8:C4:75"
#define MQTT_PUB_MOT "EPSI/SR501/5C:CF:7F:B8:C4:75"

WiFiClient WIFI_CLIENT;
PubSubClient MQTT_CLIENT(WIFI_CLIENT);
DHT dht(DHTPIN, DHTTYPE);
LiquidCrystal_PCF8574 lcd(0x27);

float temp;
float hum;
int lum;

int lcdCols = 16;
int lcdRows = 2;

float currentTime = millis();
float lastDataSend;
float lastMotionDetected;
bool motionDetected;

void setup()
{
  pinMode(LED, OUTPUT);
  pinMode(DHTPIN, INPUT);
  pinMode(MOT, INPUT);

  Serial.begin(115200);
  Wire.begin();

  lcd.begin(16, 2);
  lcd.init();
  lcd.setBacklight(LOW);

  dht.begin();

  WiFi.begin("Nek_Wifi_Room", "73967835");

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
  }
    // Affichage des données MAC
  Serial.println();
  Serial.print("ESP Board Mac Address : ");
  Serial.println(WiFi.macAddress());
  Serial.print("ESP Board IP Address : ");
  Serial.println(WiFi.localIP());
  Serial.print("ESP Board Wifi Mode : ");
  Serial.println(WiFi.getMode());
  Serial.print("Wifi Network Detected : ");
  Serial.println(WiFi.scanNetworks());
}
   // Fonction de connexion au broker MQTT
void reconnect()
{
  // Paramétrage de l'adresse et du port du broker MQTT
  MQTT_CLIENT.setServer("broker.hivemq.com", 1883);

  // Boucle qui tourne tant que la connexion au Wifi n'est pas effective

  while (!MQTT_CLIENT.connected())
  {
    // Tentative de connexion

    Serial.println("Attempt to connect to MQTT broker");
    MQTT_CLIENT.connect("esp8266_donlawiliant");

    // Attente de 2sec entre 2 tentatives de connexion

    delay(2000);
  }
  Serial.println("MQTT connected");
}
int i = 1;
int j = 1;

void loop()
{
  digitalWrite(LED, HIGH);

  // Vérification de la connexion au broker MQTT

  if (!MQTT_CLIENT.connected())
  {
    // Si la connexion au broker MQTT n'est pas réussie, on retente en boucle

    reconnect();
  }
    // Relevé de la valeur actuelle des capteurs

  float dhtTemp = dht.readTemperature();
  float dhtHum = dht.readHumidity();
  lum = analogRead(PHO);
  int motValue = digitalRead(MOT);

    // Calcul du voltage à la sortie de la photorésistance

  float lumVout = lum * 0.0048828125;

    // Calcul de la valeur en Lux en sortie de la photorésistance, 
    // en se basant sur l'intensité et l'impédence de la résistance du circuit

  int lumLux = 500 / (10 * ((5 - lumVout) / lumVout));
  currentTime = millis();
  if (motValue == HIGH)
  {
    motionDetected = true;
    Serial.println("MOTION DETECTED !");
    lastMotionDetected = millis();  
  }
  else if (lastMotionDetected + 15000 < currentTime)
  {
    motionDetected = false;
    Serial.println("NO MOTION DETECTED..");
  }

    // Préparation des payloads qui seront envoyées au broker MQTT

  String payloadTempStr = "{\"value\": " + String(dhtTemp) + ", \"unit\" : \"°C\"}";
  String payloadHumStr = "{\"value\": " + String(dhtHum) + ", \"unit\" : \"%HR\"}";
  String payloadLumStr = "{\"value\": " + String(lumLux) + ", \"unit\" : \"Lx\"}";
  String payloadMotStr = "{\"value\": " + String(motionDetected) + "}";

    // Affichage sur l'écran LCD, les 5 1ères secondes de la température et de l'humidité
    // Puis pendant les 5 secondes suivantes, de l'intensité lumineuse, ainsi que la détection de mouvement

  if ((j % 5 == 1) & (j % 10 != 1))
  {
    lcd.clear();
    lcd.setCursor(1, 0);
    lcd.print("Tem : " + String(dhtTemp) + " Cel");
    lcd.setCursor(1, 1);
    lcd.print("Hum : " + String(dhtHum) + " %HR");
  }
  else if (j % 10 == 1)
  {
    lcd.clear();
    lcd.setCursor(1, 0);
    lcd.print("Lum : " + String(lumLux) + " Lux");
    lcd.setCursor(1, 1);

      // Lorsqu'un mouvement est détecté -> illumination de l'écran LCD

    if (motionDetected == true)
    {
      lcd.setBacklight(HIGH);
      lcd.print("MOTION DETECTED");
    }
    else
    {
      lcd.setBacklight(LOW);
      lcd.print("NO MOTION DETECTED");
    }
  }
      // Si aucun valeur n'est relevé par les capteurs, on informe sur le moniteur serie

  if (isnan(dhtTemp) || isnan(dhtHum))
  {
    Serial.println("Failed to read from DHT sensor!");
  }
  else if (isnan(lum))
  {
    Serial.println("Failed to read from GL5516 sensor!");
  }

  boolean rc = MQTT_CLIENT.subscribe(MQTT_PUB_TEMP);

      // Publication des données toutes les 15 sec. vers le broker MQTT

  if (rc & ((lastDataSend + 15000) < currentTime))
  {
    Serial.println(String("Valeur payload Temp : ") + payloadTempStr);
    Serial.println(String("Valeur payload Hum : ") + payloadHumStr);
    Serial.println(String("Valeur payload Lum : ") + payloadLumStr);
    Serial.println(String("Valeur payload Mot : ") + payloadMotStr);

    Serial.println("Current time :" + String(currentTime));

      // Envoi des données au broker MQTT ~5 secondes après le boot du MCU

    if (j > 5)
    {
      Serial.println("Subscribed, now Publishing..");
      MQTT_CLIENT.publish(MQTT_PUB_TEMP, payloadTempStr.c_str());
      MQTT_CLIENT.publish(MQTT_PUB_HUM, payloadHumStr.c_str());
      MQTT_CLIENT.publish(MQTT_PUB_LUM, payloadLumStr.c_str());
      MQTT_CLIENT.publish(MQTT_PUB_MOT, payloadMotStr.c_str());

      lastDataSend = currentTime;
    }
    Serial.println("Publish N°" + String(i) + " Done !");
    i++;
  }

  j++;
  delay(1000);
  digitalWrite(LED, LOW);
}

    // Code permettant la détection de l'adresse, en hexadécimal, de l'afficheur LCD

  // byte error, address;
  // int nDevices;
  // Serial.println("Scanning...");
  // nDevices = 0;
  // for (address = 1; address < 127; address++)
  // {
  //   Wire.beginTransmission(address);
  //   error = Wire.endTransmission();
  //   if (error == 0)
  //   {
  //     Serial.print("I2C device found at address 0x");
  //     if (address < 16)
  //     {
  //       Serial.print("0");
  //     }
  //     Serial.println(address, HEX);
  //     nDevices++;
  //   }
  //   else if (error == 4)
  //   {
  //     Serial.print("Unknow error at address 0x");
  //     if (address < 16)
  //     {
  //       Serial.print("0");
  //     }
  //     Serial.println(address, HEX);
  //   }
  // }
  // if (nDevices == 0)
  // {
  //   Serial.println("No I2C devices found\n");
  // }
  // else
  // {
  //   Serial.println("done\n");
  // }