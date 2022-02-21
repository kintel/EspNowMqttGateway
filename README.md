# EspNowMqttGateway

This is an ESP-NOW to MQTT gateway.
It is inspired by, but not dependent on [plapointe6/EspMQTTClient](https://github.com/plapointe6/EspMQTTClient).

The basic idea is that the gateway will connect to a WiFi access point for internet or MQTT connectivity.
Once connected, it will allow clients to scan over ESP-NOW to discover the MAC address and WiFi channel of the gateway.

Once a WiFi channel has been established, clients can send `topicname=topicstring` or `topicname:=topicstring` messages over ESP-NOW, and the gateway will relay that over MQTT. The latter format is for retained mode MQTT messages.
