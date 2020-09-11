#include "WIFI.h"

#include <ESP8266mDNS.h>

#include "Constants.h"
#include "GUI.h"
#include "MQTT.h"
#include "Settings.h"

namespace WIFI
{
    namespace Callback
    {
        void onConnect(const WiFiEventStationModeGotIP& event)
        {
            GUI::updateWiFiStatus(FPSTR(CONNECTED));
            MQTT::update();
        }

        void onDisconnect(const WiFiEventStationModeDisconnected& event)
        {
            GUI::updateWiFiStatus(FPSTR(DISCONNECTED));
            MQTT::reconnectTimer.detach(); // ensure we don't reconnect to MQTT while reconnecting to Wi-Fi
            reconnectTimer.once_scheduled(RECONNECT_DELAY, WIFI::connect);
        }
    } // namespace Callback

    void setup()
    {
        // WiFi.onStationModeConnected();
        WiFi.onStationModeGotIP(WIFI::Callback::onConnect);
        WiFi.onStationModeDisconnected(WIFI::Callback::onDisconnect);
    }

    String macAddress()
    {
        uint8_t macAr[6];
        WiFi.macAddress(macAr);
        char macStr[13] = {0};
        sprintf_P(macStr, PSTR("%02X%02X%02X%02X%02X%02X"), macAr[0], macAr[1], macAr[2], macAr[3], macAr[4], macAr[5]);
        return String(macStr);
    }

    bool connectToAP(const char* ssid, const char* password)
    {
        // Disconnect everything we can, this fixes an issue where the wifi just does not connect
        WiFi.disconnect();
        WiFi.softAPdisconnect();

        // Explicitly set the ESP8266 to be a WiFi-client, otherwise, it by default,
        // would try to act as both a client and an access-point and could cause
        // network-issues with your other WiFi-devices on your WiFi-network.
        WiFi.mode(WIFI_STA);

        // Set hostname
#if defined(ESP32)
        WiFi.setHostname(FPSTR(hostname));
#else
        WiFi.hostname(FPSTR(hostname));
#endif

        // try to connect to existing network
        WiFi.begin(ssid, password);

        // Wait until we connected or failed to connect
        wl_status_t state = WiFi.status();
        while (state != WL_CONNECTED && state != WL_CONNECT_FAILED && state != WL_NO_SSID_AVAIL)
        {
            // delay(500);
            optimistic_yield(500);
            state = WiFi.status();
        }

        // Setup DNS so we don't have to find and type the ip address
        MDNS.begin(FPSTR(hostname));

        const bool connected = state == WL_CONNECTED;
        if (connected)
        {
            WiFi.setAutoReconnect(true);
        }

        switch (state)
        {
        case WL_CONNECT_FAILED:
            GUI::updateWiFiStatus("Connection failed");
            break;
        case WL_NO_SSID_AVAIL:
            GUI::updateWiFiStatus("SSID unavailable");
            break;

        default:
            break;
        }

        return connected;
    }

    void createAP()
    {
        WiFi.mode(WIFI_AP);
        WiFi.softAPConfig(accessPointIP, accessPointIP, netmask);
        WiFi.softAP(hostname);

        uint8_t timeout = 5;
        do
        {
            delay(500);
        } while (--timeout);
    }

    // void connect(const char* ssid, const char* password)
    // {
    //     // if not connected -> create hotspot
    //     if (!connectToAP(ssid, password))
    //     {
    //         createAP();
    //     }
    // }

    void connect()
    {
        const int scanResult = WiFi.scanComplete();
        if (scanResult == WIFI_SCAN_RUNNING || !Settings::settings.wifi)
        {
            // If the wifi scan is still running just wait a little longer
            reconnectTimer.once_scheduled(RECONNECT_DELAY, WIFI::connect);
        }
        else if (scanResult == WIFI_SCAN_FAILED)
        {
            WiFi.scanNetworks(true);
            reconnectTimer.once_scheduled(RECONNECT_DELAY, WIFI::connect);
        }
        else
        {
            bool connected = false;
            for (int item = 0; item < scanResult; item++)
            {
                const String foundSSID = WiFi.SSID(item);

                // Make sure we only try to connect if we found the correct SSID
                if (foundSSID.equals(Settings::settings.ssid))
                {
                    GUI::updateWiFiStatus("Connecting");
                    connected = connectToAP(Settings::settings.ssid, Settings::settings.password);
                    break;
                }
            }
            // This will clear ram and make sure that next call of scanComplete will return WIFI_SCAN_FAILED
            WiFi.scanDelete();

            if (!connected)
            {
                if (WiFi.getMode() != WIFI_AP)
                {
                    createAP();
                }
            }
        }
    }

    const String mac = macAddress();
    // Access point settings
    const IPAddress accessPointIP(192, 168, 1, 1);
    const IPAddress netmask(255, 255, 255, 0);

    Ticker reconnectTimer;

} // namespace WIFI
