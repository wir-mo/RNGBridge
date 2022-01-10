#pragma once

#include <PubSubClient.h>

#include "WIFI.h"

namespace MQTT
{
    /**
     * @brief Setup the mqtt client
     * TODO
     */
    extern void setup();

    extern void update();

    extern void updateIP(const IPAddress& ip);

    extern void updatePort(const uint16_t port);

    extern void updateIPPort(const IPAddress& ip, const uint16_t port);

    extern void connect();

    extern void disconnect();

    extern void publish(const String& payload, uint8_t qos = 0, bool retain = false);
    extern void publish(const char* payload, uint8_t qos = 0, bool retain = false);
    extern void publish(const char* topic, const char* payload, uint8_t qos = 0, bool retain = false);

    extern const char* lastWillFormat PROGMEM;
    extern const char* connectionMsgFormat PROGMEM;
    extern WiFiClient espClient;
    extern PubSubClient mqtt;
    extern bool connected;
    extern bool _setup;
} // namespace MQTT
