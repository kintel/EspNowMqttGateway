#include "EspNowMQTTClient.h"
#include <esp_wifi.h>
#include <WiFi.h>

EspNowMQTTClient *EspNowMQTTClient::inst = nullptr;

EspNowMQTTClient::EspNowMQTTClient()
{
  EspNowMQTTClient::inst = this;
}

// Check if a packet is a scan packet.
// A scan packet is a 11-byte packed with the first 4 bytes being hex 0x5caff01d ("scaffold").
bool isScanAckPacket(const uint8_t *packet, int packet_len) {
  return packet_len == 11 && *(uint32_t *)packet == 0x5caff01d;
}

// Broadcast a scan packet.
// A scan packet is a 32-bit packed with the hex code 0x5caff01d ("scaffold").
void broadcastScanPacket() {
  const uint8_t broadcast[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
  const uint32_t scaffold = 0x5caff01d;
  const auto err = esp_now_send(broadcast, (const uint8_t *)&scaffold, 4);
  if (err != ESP_OK) {
    Serial.print("sendScan() failed: "); Serial.println(String(esp_err_to_name(err)));

  }
}

void EspNowMQTTClient::onScanPacketRecv(const uint8_t *mac_addr, const uint8_t *packet, int packet_len) {
  Serial.println("Got packet");
  if (isScanAckPacket(packet, packet_len)) {
    Serial.print("Got scan ack packet on channel ");
    Serial.println(packet[10]);

    inst->syncedChannel = packet[10];
    const uint8_t *mac = packet + 4;
    for (int i = 0; i < 6; i++) inst->gateway.peer_addr[i] = mac[i];
    inst->gateway.channel = packet[10];
    if (esp_now_is_peer_exist(inst->gateway.peer_addr)) {
      esp_now_del_peer(inst->gateway.peer_addr);
    }
    auto addErr = esp_now_add_peer(&inst->gateway);
    if (addErr != ESP_OK) {
      Serial.print("esp_now_add_peer(gateway) failed: ");
      Serial.println(String(esp_err_to_name(addErr)));
      return;
    }

//    Serial.print("MAC: ");
//    const uint8_t *mac = packet + 4;
//    char macStr[18] = { 0 };
//    sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
//    Serial.println(String(macStr));
  }
}

void EspNowMQTTClient::onScanPacketSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  inst->syncPacketSent = true;
}

// Scans for a gateway and returns the channel at which a gateway was found,
// or 0 if no gateway was found.
// When returning non-zero, the ESP will be configured to send at the returned channel.
int EspNowMQTTClient::scan(int timeout)
{
  this->syncedChannel = 0;
  this->connected = false;
  this->failCounter = 0;
  auto startTime = millis();
  
  // Setup peer for broadcast scanning
  esp_now_peer_info_t scan_peer = {
    {0xff, 0xff, 0xff, 0xff, 0xff, 0xff},
    {}, 0, WIFI_IF_STA, false, NULL
  };
  if (esp_now_is_peer_exist(scan_peer.peer_addr)) {
    esp_now_del_peer(scan_peer.peer_addr);
  }
  esp_err_t err = esp_now_add_peer(&scan_peer);
  if (err != ESP_OK) {
    Serial.print("esp_now_add_peer(scan_peer) failed: ");
    Serial.println(String(esp_err_to_name(err)));
    return false;
  }

  esp_now_register_recv_cb(EspNowMQTTClient::onScanPacketRecv);
  esp_now_register_send_cb(EspNowMQTTClient::onScanPacketSent);

  bool firstRetry = true;
  bool didTimeOut = false;
  while (!didTimeOut && !this->syncedChannel) {
    for (uint8_t ch = 1; ch <= 14 && !this->syncedChannel; ch++) {
      esp_wifi_set_channel(ch, WIFI_SECOND_CHAN_NONE);
      this->syncPacketSent = false;
      broadcastScanPacket();
      while (!this->syncPacketSent) {
        delay(100);
      }
    }
    // try again after 100ms
    if (!this->syncedChannel) {
      if (firstRetry) {
        Serial.print("Gateway not found. Trying again");
        firstRetry = false;
      }
      else {
        Serial.print(".");
      }
      delay(100);
    }
    didTimeOut = (millis() - startTime) > timeout;
  }
  Serial.println();

  esp_now_unregister_recv_cb();
  esp_now_unregister_send_cb();

  if (this->syncedChannel) {
    Serial.print("Connected on channel "); Serial.println(this->syncedChannel); 
    esp_wifi_set_channel(this->syncedChannel, WIFI_SECOND_CHAN_NONE);
    esp_now_register_recv_cb(EspNowMQTTClient::onMQTTPacketRecv);
    esp_now_register_send_cb(EspNowMQTTClient::onMQTTPacketSent);
    this->connected = true;
    this->lastTransmission = millis();
  }
  else {
    Serial.println("Unable to find gateway");
  }

  return this->syncedChannel;
}

void EspNowMQTTClient::onMQTTPacketRecv(const uint8_t *mac_addr, const uint8_t *packet, int packet_len) {
}

void EspNowMQTTClient::onMQTTPacketSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  inst->mqttPacketStatus = status;
}

int EspNowMQTTClient::begin(const String& ssid, int timeout)
{
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESPNow Init failed");
    return 0;
  }

  return this->scan(timeout);
}

// Publish an MQTT value over ESP-NOW
// Returns true if message was acked by the gateway, false otherwise.
bool EspNowMQTTClient::publish(const String &topic, const String &payload, bool retain)
{
  // ESP-NOW message format:
  // Regular messages: "topicstring=valuestring\0"
  // Retained messages: "topicstring:=valuestring\0"
  String packet = topic + (retain ? ":" : "") + "=" + payload;
  Serial.println("Publish: " + packet);
  this->mqttPacketStatus = -1;
  esp_err_t err = esp_now_send(this->gateway.peer_addr, (uint8_t *)packet.c_str(), packet.length() + 1);
  if (err != ESP_OK) {
    Serial.print("Publish failed: ");
    Serial.println(String(esp_err_to_name(err)));
  }
  while (this->mqttPacketStatus == -1) {
    delay(1);
  }
  if (this->mqttPacketStatus != 0) this->failCounter++;
  else {
    this->failCounter = 0;
    this->lastTransmission = millis();
  }
  
  return this->mqttPacketStatus == 0;
}

bool EspNowMQTTClient::isConnected() const
{
  return this->connected;
}

void EspNowMQTTClient::loop()
{
  if (this->failCounter > 10) {
    Serial.println("Too many failures; re-scanning for gateway");
    this->scan();
  }
  else {
    if (!this->connected && millis() - this->lastTransmission > 60000) {
      this->scan();
      this->lastTransmission = millis();
    }
  }
}
