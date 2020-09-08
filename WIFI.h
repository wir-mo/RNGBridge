#pragma once

#include <ESP8266WiFi.h>
#include <IPAddress.h>
#include <String.h>
#include <Ticker.h>
#include <WString.h>

namespace WIFI
{
    namespace Callback
    {
        extern void onConnect(const WiFiEventStationModeGotIP &event);

        extern void onDisconnect(const WiFiEventStationModeDisconnected &event);
    } // namespace Callback

    extern void setup();

    /**
     * @brief Get the mac address without any format
     *
     * @return String Containing the mac
     */
    extern String macAddress();

    /**
     * @brief Connect to an access point with the given ssid and password
     *
     * @param ssid The ssid of the AP
     * @param password The password of the AP
     * @return true If the connection was successfull
     * @return false If the connection failed
     */
    extern bool connectToAP(const String &ssid, const String &password);

    /**
     * @brief Create an access point
     *
     */
    extern void createAP();

    /**
     * @brief Connect to an access point or fallback to own access point
     *
     * If no connection can be established to the given acces point ssid a new access point is created/hosted
     *
     * @param ssid The ssid of the AP to connect to
     * @param password The password of the AP to connect to
     */
    extern void connect(const String &ssid, const String &password);

    extern void connectToWifi();

    constexpr uint8_t RECONNECT_DELAY = 5;
    extern const String mac;
    // Access point settings
    extern const IPAddress accessPointIP;
    extern const IPAddress netmask;

    extern Ticker reconnectTimer;
} // namespace WIFI