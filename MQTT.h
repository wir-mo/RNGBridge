#pragma once

#include <PangolinMQTT.h>
#include <Ticker.h>

#include "WIFI.h"

namespace MQTT
{
    namespace Callback
    {
        extern void onConnect(bool sessionPresent);

        extern void onDisconnect(int8_t reason);
    } // namespace Callback

    /**
     * @brief Setup the mqtt client
     * TODO
     */
    extern void setup();

    void update();

    extern void updateTopic();

    extern void updateIP(const IPAddress& ip);

    extern void updatePort(const uint16_t port);

    extern void updateIPPort(const IPAddress& ip, const uint16_t port);

    extern void connect();

    extern void disconnect();

    extern void publish(const String& payload, uint8_t qos = 0, bool retain = false);
    extern void publish(const char* payload, uint8_t qos = 0, bool retain = false);

    extern const char* lastWillFormat PROGMEM;
    extern const char* connectionMsgFormat PROGMEM;
    extern PangolinMQTT mqtt;
    extern Ticker reconnectTimer;
    extern bool connected;
    extern bool _setup;
} // namespace MQTT
