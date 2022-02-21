#include "EspNowMQTTServer.h"
#include <esp_now.h>
#include <esp_wifi.h>
#include <WiFi.h>

// Check if a packet is a scan packet.
// A scan packet is a 32-bit packed with the hex code 0x5caff01d ("scaffold").
bool isScanPacket(const uint8_t *packet, int packet_len)
{
  return packet_len == 4 && *(uint32_t *)packet == 0x5caff01d;
}

void sendScanAck(const uint8_t *dst_addr)
{
  esp_now_peer_info_t scan_client {
    {dst_addr[0], dst_addr[1], dst_addr[2], dst_addr[3], dst_addr[4], dst_addr[5]},
    {}, (uint8_t)WiFi.channel(), WIFI_IF_STA, false, NULL
  };
  if (esp_now_is_peer_exist(scan_client.peer_addr)) {
    esp_now_del_peer(scan_client.peer_addr);
  }
  auto err = esp_now_add_peer(&scan_client);
  if (err != ESP_OK) {
    Serial.print("esp_now_add_peer(scan_client) failed: ");
    Serial.println(String(esp_err_to_name(err)));
    return;
  }

  // FIXME: We could describe this as a struct instead
  uint8_t packet[11] = {
    0x1d, 0xf0, 0xaf, 0x5c,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    (uint8_t)WiFi.channel(),
  };
  WiFi.macAddress(packet+4);
  
  auto result = esp_now_send(dst_addr, packet, 11);
  if (result != ESP_OK) {
    Serial.print("Send scan ACK failed: ");
    Serial.println(String(esp_err_to_name(result)));
  }
}

bool handleScanPacket(const uint8_t *mac_addr, const uint8_t *packet, int packet_len)
{
  if (isScanPacket(packet, packet_len)) {
    sendScanAck(mac_addr);
    return true;
  }
  return false;
}

bool handleMQTTPacket(const uint8_t *mac_addr, const uint8_t *packet, int packet_len)
{
  // ESP-NOW MQTT message format:
  // Regular messages: "topicstring=valuestring\0"
  // Retained messages: "topicstring:=valuestring\0"

  String payload((const char *)packet);

  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  /*
  Serial.println("");
  Serial.print("Recv from: "); Serial.println(macStr);
  Serial.print("["); Serial.print(packet_len); Serial.print("]: ");
  Serial.println(payload);
  */

  bool retain = false;
  int eq = payload.indexOf('=');
  if (eq <= 0) {
    Serial.println("Unknown packet: " + payload);
    return false;
  }
  String value = payload.substring(eq+1);
  if (payload.charAt(eq-1) == ':') {
    retain = true;
    eq--;
  }
  String topic = payload.substring(0, eq);

  onMessageReceived(String(macStr), topic, value, retain);
  return true;
}

void onDataRecv(const uint8_t *mac_addr, const uint8_t *packet, int packet_len)
{
  if (handleScanPacket(mac_addr, packet, packet_len)) return;
  handleMQTTPacket(mac_addr, packet, packet_len);
}

EspNowMQTTServer::EspNowMQTTServer()
{
}

// Starts an ESP-NOW MQTT server.
// Must be called _after_ a connection to a WiFi access point is made
bool EspNowMQTTServer::begin(const String& ssid)
{
  // This is needed since power saving is enabled by default in STA mode.
  // If we use WIFI_AP_STA, this is not needed.
  esp_wifi_set_ps(WIFI_PS_NONE);
  this->channel = WiFi.channel();
  Serial.print("Connected on WiFi channel ");
  Serial.println(this->channel);

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESPNow Init failed");
    return false;
  }
  esp_now_register_recv_cb(onDataRecv);
  return true;
}

void EspNowMQTTServer::loop()
{
}
