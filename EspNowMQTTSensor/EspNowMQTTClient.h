#pragma once

#include <Arduino.h>
#include <esp_now.h>

// Default timeout is 10s
const unsigned int SCAN_TIMEOUT = 10000;

class EspNowMQTTClient
{
  static EspNowMQTTClient *inst;
  
  esp_now_peer_info_t gateway;
  int syncedChannel = 0;
  bool syncPacketSent = false;
  int mqttPacketStatus;
  bool connected = false;
  int failCounter = 0;
  unsigned int lastTransmission = 0;
  
  bool ScanForGateway(const String& ssid);
  int scan(int timeout = SCAN_TIMEOUT);
  static void onScanPacketRecv(const uint8_t *mac_addr, const uint8_t *packet, int packet_len);
  static void onScanPacketSent(const uint8_t *mac_addr, esp_now_send_status_t status);
  static void onMQTTPacketRecv(const uint8_t *mac_addr, const uint8_t *packet, int packet_len);
  static void onMQTTPacketSent(const uint8_t *mac_addr, esp_now_send_status_t status);
public:
  EspNowMQTTClient();

  int begin(const String& ssid, int timeout = SCAN_TIMEOUT);
  void loop();
  bool publish(const String &topic, const String &payload, bool retain=false);
  bool isConnected() const;
};
