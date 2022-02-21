#include <Arduino.h>

// MUST be implemented in your sketch. Called when an MQTT message arrives over ESP-NOW
void onMessageReceived(const String &macAddress, const String &topic, const String &value, bool retain);

class EspNowMQTTServer
{
  int channel = 0;

public:
  EspNowMQTTServer();

  bool begin(const String& ssid);
  void loop();
};
