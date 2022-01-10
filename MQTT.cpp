#include "MQTT.h"

#include "Constants.h"
#include "GUI.h"
#include "Settings.h"

namespace MQTT
{
    void setup()
    {
        GUI::updateMQTTStatus(FPSTR(DISCONNECTED));
        const IPAddress ip = Settings::settings.mqttIP;
        updateIPPort(ip, Settings::settings.mqttPort);

        _setup = true;
    }

    void update()
    {
        if (_setup)
        {
            if (Settings::settings.mqtt)
            {
                GUI::updateMQTTStatus(F("Started"));
                if (!mqtt.connected())
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
                    else if (!portSet)
                    {
                        GUI::updateMQTTStatus(F("Port wrong"));
                    }
                    connect();
                }
            }
            else
            {
                if (mqtt.connected())
                {
                    disconnect();
                }
                GUI::updateMQTTStatus(F("Stopped"));
            }
        }
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
            mqtt.disconnect();
            // TODO maybe need call to connect here
        }
    }

    void connect()
    {
        // Set last will and connect
        char will[44];
        snprintf_P(will, 44, lastWillFormat, WIFI::mac.c_str());
        const bool connected = mqtt.connect(HOSTNAME, Settings::settings.mqttUser, Settings::settings.mqttPass,
            Settings::settings.topic, 2, true, will);

        if (connected)
        {
            // Publish connected message
            char connectionMsq[43];
            snprintf_P(connectionMsq, 43, connectionMsgFormat, WIFI::mac.c_str());
            MQTT::publish(connectionMsq, 2, true);

            // Update GUI
            GUI::updateMQTTStatus(FPSTR(CONNECTED));
        }
        else
        {
            // Update GUI
            GUI::updateMQTTStatus(F("Could not connect"));
        }
    }

    void disconnect()
    {
        mqtt.disconnect();
        GUI::updateMQTTStatus(FPSTR(DISCONNECTED));
    }

    void publish(const String& payload, uint8_t qos, bool retain)
    {
        // At most once (0)
        // At least once (1)
        // Exactly once (2)
        if (mqtt.connected())
        {
            mqtt.publish(
                Settings::settings.topic, reinterpret_cast<const uint8_t*>(payload.c_str()), payload.length(), retain);
        }
    }

    void publish(const char* payload, uint8_t qos, bool retain)
    {
        publish(Settings::settings.topic, payload, qos, retain);
    }

    void publish(const char* topic, const char* payload, uint8_t qos, bool retain)
    {
        // At most once (0)
        // At least once (1)
        // Exactly once (2)
        if (mqtt.connected())
        {
            mqtt.publish(topic, reinterpret_cast<const uint8_t*>(payload), strlen(payload), retain);
        }
    }

    const char* lastWillFormat PROGMEM = R"({"device":"%s","connected":false})";
    const char* connectionMsgFormat PROGMEM = R"({"device":"%s","connected":true})";
    WiFiClient espClient;
    PubSubClient mqtt(espClient);
    bool connected = false;
    bool _setup = false;
} // namespace MQTT
