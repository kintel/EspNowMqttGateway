#include "EspNowMQTTClient.h"
#include <WiFi.h>

EspNowMQTTClient client;

void setup() {
  Serial.begin(115200);
  Serial.println("Hello ESP-NOW Sensor");

  auto startTime = millis();
  int channel = client.begin("MQTT-gateway");
  auto endTime = millis();
  if (channel) {
    Serial.print("Gateway found on channel "); Serial.print(channel);
    Serial.print(" in ");
    Serial.print(endTime - startTime); Serial.println(" ms");
  }
}

int dummy_sensor = 0;

void loop() {
  client.loop();
  if (client.isConnected()) {
    if (!client.publish("SensorName", String(dummy_sensor++))) {
      Serial.println("Publish failed");
    }
  }
  delay(500);
}
