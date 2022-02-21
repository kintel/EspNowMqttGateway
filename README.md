# EspNowMqttGateway

This is an ESP-NOW to MQTT gateway.

The basic idea is that the gateway will connect to a WiFi access point for internet or MQTT connectivity.
Once connected, it will allow clients to scan over ESP-NOW to discover the MAC address and WiFi channel of the gateway.

Once a WiFi channel has been established, clients can send `topicname=topicstring` or `topicname:=topicstring` messages over ESP-NOW, and the gateway can relay that over MQTT. The latter format is for retained mode MQTT messages.

The example uses [plapointe6/EspMQTTClient](https://github.com/plapointe6/EspMQTTClient) for MQTT connectivity but messaged can be relayed using any MQTT library, and is not even tied to MQTT in any way.
