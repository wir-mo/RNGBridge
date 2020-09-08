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

    extern void updateTopic();

    extern void updateIP(const IPAddress ip);

    extern void updatePort(const uint16_t port);

    extern void updateIPPort(const IPAddress ip, const uint16_t port);

    extern void updateTopic(const String& topic);

    extern void connect();

    extern void publish(const String& payload, bool retain = false);

    extern const String lastWillMsg;
    extern PangolinMQTT mqtt;
    extern String topic;
    extern Ticker reconnectTimer;
} // namespace MQTT
