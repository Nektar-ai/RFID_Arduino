#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#define LED 15
#define LED2 2

WiFiClient WIFI_CLIENT;
PubSubClient MQTT_CLIENT(WIFI_CLIENT);

void setup() {
  Serial.begin(115200);
  
  pinMode(LED, OUTPUT);
  pinMode(LED2, OUTPUT);

  WiFi.begin("Nek_Wifi_Room", "73967835");

    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
  }

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

// This function connects to the MQTT broker
void reconnect() {
  // Set our MQTT broker address and port
  MQTT_CLIENT.set_server("broker.hivemq.com", 1883);
  
  // Loop until we're reconnected
  while (!MQTT_CLIENT.connected()) {
    // Attempt to connect
    Serial.println("Attempt to connect to MQTT broker");
    MQTT_CLIENT.connect("esp8266_donlawiliant");

    // Wait some time to space out connection requests
    delay(3000);
  }
  Serial.println("MQTT connected");
}

void loop() {

    // Check if we're connected to the MQTT broker
  if (!MQTT_CLIENT.connected()) {
    // If we're not, attempt to reconnect
    reconnect();
  }

  if (MQTT_CLIENT.connect("esp8266_donlawiliant")) {
    // connection succeeded
    Serial.println("Connected now subscribing");
    boolean r= MQTT_CLIENT.subscribe("test");

  } 
  else {
    // connection failed
    // mqttClient.state() will provide more information
    // on why it failed.
    Serial.println("Connection failed ");
  }

  Serial.println("Loop...");
  digitalWrite(LED, HIGH);
  digitalWrite(LED2, HIGH);
  delay(1000);
  digitalWrite(LED, LOW);
  digitalWrite(LED2, LOW);
  delay(1000);
}
