#include "WIFI.h"

#include "Constants.h"

#ifdef HAVE_GUI
#include "GUI.h"
#endif

#include "MQTT.h"
#include "Settings.h"

namespace WIFI
{
    namespace Callback
    {
        void onConnect(const WiFiEventStationModeGotIP &event)
        {
#ifdef HAVE_GUI
            GUI::updateWiFiStatus(FPSTR(CONNECTED));
#endif
            MQTT::connect();
        }

        void onDisconnect(const WiFiEventStationModeDisconnected &event)
        {
#ifdef HAVE_GUI
            GUI::updateWiFiStatus(FPSTR(DISCONNECTED));
#endif
            MQTT::reconnectTimer.detach(); // ensure we don't reconnect to MQTT while reconnecting to Wi-Fi
            reconnectTimer.once(RECONNECT_DELAY, connectToWifi);
        }
    } // namespace Callback

    void setup()
    {
#if defined(ESP32)
        WiFi.setHostname(FPSTR(hostname));
#else
        WiFi.hostname(FPSTR(hostname));
#endif
        // WiFi.onStationModeConnected();
        WiFi.onStationModeGotIP(WIFI::Callback::onConnect);
        WiFi.onStationModeDisconnected(WIFI::Callback::onDisconnect);
    }

    String macAddress()
    {
        uint8_t macAr[6];
        WiFi.macAddress(macAr);
        char macStr[13] = {0};
        sprintf(macStr, "%02X%02X%02X%02X%02X%02X", macAr[0], macAr[1], macAr[2], macAr[3], macAr[4], macAr[5]);
        return String(macStr);
    }

    bool connectToAP(const String &ssid, const String &password)
    {
        if (WiFi.isConnected())
        {
            WiFi.disconnect();
        }
        else
        {
            WiFi.softAPdisconnect();
        }
        /* Explicitly set the ESP8266 to be a WiFi-client, otherwise, it by default,
              would try to act as both a client and an access-point and could cause
              network-issues with your other WiFi-devices on your WiFi-network. */
        WiFi.mode(WIFI_STA);
        // try to connect to existing network
        WiFi.begin(ssid.c_str(), password.c_str());

        // Wait until we connected or failed to connect
        uint8_t state = WiFi.status();
        while (true)
        {
            if (state == WL_CONNECTED || state == WL_CONNECT_FAILED || state == WL_NO_SSID_AVAIL)
            {
                break;
            }
            delay(500);
            state = WiFi.status();
        }
        const bool connected = state == WL_CONNECTED;
        if (connected)
        {
            WiFi.setAutoReconnect(true);
        }

#ifdef HAVE_GUI
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
#endif

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

    void connect(const String &ssid, const String &password)
    {
        // if not connected -> create hotspot
        if (!connectToAP(ssid, password))
        {
            createAP();
        }
    }

    void connectToWifi()
    {
        const int scanResult = WiFi.scanComplete();
        if (scanResult == WIFI_SCAN_RUNNING)
        {
            // If the wifi scan is still running just wait a little longer
            reconnectTimer.once(RECONNECT_DELAY, connectToWifi);
        }
        else if (scanResult == WIFI_SCAN_FAILED)
        {
            WiFi.scanNetworks(true);
        }
        else
        {
            for (int item = 0; item < scanResult; item++)
            {
                const String foundSSID = WiFi.SSID(item);

                // Make sure we only try to connect if we found the correct SSID
                if (foundSSID.equals(Settings::settings.ssid))
                {
                    connect(String(Settings::settings.ssid), String(Settings::settings.password));
                    break;
                }
            }
            // This will clear ram and make sure that next call of scanComplete will return WIFI_SCAN_FAILED
            WiFi.scanDelete();
        }
    }

    const String mac = macAddress();
    // Access point settings
    const IPAddress accessPointIP(192, 168, 1, 1);
    const IPAddress netmask(255, 255, 255, 0);

    Ticker reconnectTimer;

} // namespace WIFI
