#include <EspMQTTClient.h>
#include "EspNowMQTTServer.h"

EspMQTTClient client(
  "WifiSSID",
  "WifiPassword",
  "my-project-cloud.shiftr.io", // MQTT Broker server
  "MQTTUsername",               // Can be omitted if not needed
  "MQTTPassword",               // Can be omitted if not needed
  "TestClient",                 // Client name that uniquely identify your device
  1883                          // The MQTT port, default to 1883. this line can be omitted
);

EspNowMQTTServer server;

unsigned long lastMillis = 0;

void setup()
{
  Serial.begin(115200);
  Serial.println("Hello MQTT-Gateway");
  
  client.enableDebuggingMessages(); // Enable debugging messages sent to serial output
}

void onConnectionEstablished()
{
  server.begin("MQTT-gateway");
}

void onMessageReceived(const String &macAddress, const String &topic, const String &value, bool retain) {
  client.publish(topic, value, retain);
}
  
void loop()
{
  client.loop();
  server.loop();

  if (millis() - lastMillis > 10000) {
    lastMillis = millis();

    client.publish("MQTT-Gateway/uptime", String(millis()/1000), 1);
    client.publish("MQTT-Gateway/MAC", WiFi.macAddress(), 1);
  }
}
