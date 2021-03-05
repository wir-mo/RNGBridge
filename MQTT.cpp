#include "Constants.h"
#include "GUI.h"
#include "MQTT.h"
#include "Settings.h"

namespace MQTT
{

    namespace Callback
    {
        void onConnect(bool sessionPresent)
        {
            // Indicate we are connected
            connected = true;

            // Update GUI
            GUI::updateMQTTStatus(FPSTR(CONNECTED));

            // Send connected message
            char connectionMsq[43];
            snprintf_P(connectionMsq, 43, connectionMsgFormat, WIFI::mac.c_str());
            MQTT::publish(connectionMsq, 2, true);
        }

        void onDisconnect(int8_t reason)
        {
            // Indicate we are disconnected
            connected = false;

            // Update GUI
            GUI::updateMQTTStatus(FPSTR(DISCONNECTED));

            // If WiFi is connected and mqtt is enabled try to reconnect
            if (WiFi.isConnected() && Settings::settings.mqtt)
            {
                reconnectTimer.once_scheduled(2, connect);
            }
        }
    } // namespace Callback

    void setup()
    {
        GUI::updateMQTTStatus(FPSTR(DISCONNECTED));
        const IPAddress ip = Settings::settings.mqttIP;
        updateIPPort(ip, Settings::settings.mqttPort);
        updateTopic();
        updateCredentials();
        mqtt.setClientId(FPSTR(hostname));
        mqtt.onConnect(MQTT::Callback::onConnect);
        mqtt.onDisconnect(MQTT::Callback::onDisconnect);

        _setup = true;
    }

    void update()
    {
        if (_setup)
        {
            if (Settings::settings.mqtt)
            {
                GUI::updateMQTTStatus(F("Started"));
                if (!connected)
                {
                    const IPAddress ip = Settings::settings.mqttIP;
                    const bool ipSet = ip.isSet();
                    const bool portSet = Settings::settings.mqttPort > 0;
                    if (!ipSet && !portSet)
                    {
                        GUI::updateMQTTStatus(F("IP and Port wrong"));
                    }
                    else if (!ipSet)
                    {
                        GUI::updateMQTTStatus(F("IP wrong"));
                    }
                    else
                    {
                        GUI::updateMQTTStatus(F("Port wrong"));
                    }
                    connect();
                }
            }
            else
            {
                if (connected)
                {
                    disconnect();
                }
                GUI::updateMQTTStatus(F("Stopped"));
            }
        }
    }

    void updateTopic()
    {
        char will[44];
        snprintf_P(will, 44, lastWillFormat, WIFI::mac.c_str());
        mqtt.setWill(Settings::settings.topic, 2, true, will);
    }

    void updateIP(const IPAddress& ip) { updateIPPort(ip, Settings::settings.mqttPort); }

    void updatePort(const uint16_t port)
    {
        const IPAddress ip = Settings::settings.mqttIP;
        updateIPPort(ip, port);
    }

    void updateIPPort(const IPAddress& ip, const uint16_t port)
    {
        if (ip.isSet() && port > 0)
        {
            mqtt.setServer(ip, port);
            mqtt.disconnect(true);
            // TODO maybe need call to connect here
        }
    }

    void updateCredentials() { mqtt.setCredentials(Settings::settings.mqttUser, Settings::settings.mqttPass); }

    void connect() { mqtt.connect(); }

    void disconnect() { mqtt.disconnect(true); }

    void publish(const String& payload, uint8_t qos, bool retain)
    {
        // At most once (0)
        // At least once (1)
        // Exactly once (2)
        if (connected)
        {
            mqtt.publish(Settings::settings.topic, 0, retain, payload);
        }
    }

    void publish(const char* payload, uint8_t qos, bool retain)
    {
        // At most once (0)
        // At least once (1)
        // Exactly once (2)
        if (connected)
        {
            mqtt.publish(Settings::settings.topic, 0, retain, (uint8_t*)payload, strlen(payload), false);
        }
    }

    const char* lastWillFormat PROGMEM = R"({"device":"%s","connected":false})";
    const char* connectionMsgFormat PROGMEM = R"({"device":"%s","connected":true})";
    PangolinMQTT mqtt;
    Ticker reconnectTimer;
    bool connected = false;
    bool _setup = false;
} // namespace MQTT
