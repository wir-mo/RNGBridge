#include "MQTT.h"

#include "Constants.h"

#ifdef HAVE_GUI
#include "GUI.h"


#endif

#include "Settings.h"

namespace MQTT
{

    namespace Callback
    {
        void onConnect(bool sessionPresent)
        {
#ifdef HAVE_GUI
            GUI::updateMQTTStatus(FPSTR(CONNECTED));
#endif

            char connectionMsq[43];
            snprintf_P(connectionMsq, 43, connectionMsgFormat, WIFI::mac.c_str());
            MQTT::publish(connectionMsq, 2, true);
        }

        void onDisconnect(int8_t reason)
        {
#ifdef HAVE_GUI
            GUI::updateMQTTStatus(FPSTR(DISCONNECTED));
#endif

            if (WiFi.isConnected())
            {
                reconnectTimer.once(2, connect);
            }
        }
    } // namespace Callback

    void setup()
    {
#ifdef HAVE_GUI
        GUI::updateMQTTStatus(FPSTR(DISCONNECTED));
#endif

        const IPAddress ip = Settings::settings.mqttIP;
        const uint16_t port = Settings::settings.mqttPort;
        updateIPPort(ip, port);
        updateTopic();
        mqtt.setClientId(FPSTR(hostname));
        mqtt.onConnect(MQTT::Callback::onConnect);
        mqtt.onDisconnect(MQTT::Callback::onDisconnect);
    }

    void updateTopic()
    {
        char will[44];
        snprintf_P(will, 44, lastWillFormat, WIFI::mac.c_str());
        mqtt.setWill(topic.c_str(), 2, true, will);
    }

    void updateIP(const IPAddress ip) { updateIPPort(ip, Settings::settings.mqttPort); }

    void updatePort(const uint16_t port)
    {
        const IPAddress ip = Settings::settings.mqttIP;
        updateIPPort(ip, port);
    }

    void updateIPPort(const IPAddress ip, const uint16_t port)
    {
        if (ip.isSet() && port > 0)
        {
            mqtt.setServer(ip, port);
        }
    }

    void updateTopic(const String& topic) { MQTT::topic = topic; }

    void connect() { mqtt.connect(); }

    void publish(const String& payload, uint8_t qos, bool retain)
    {
        // At most once (0)
        // At least once (1)
        // Exactly once (2)
        mqtt.publish(topic.c_str(), 0, retain, payload);
    }

    void publish(const char* payload, uint8_t qos, bool retain)
    {
        // At most once (0)
        // At least once (1)
        // Exactly once (2)
        mqtt.publish(topic.c_str(), 0, retain, (uint8_t*)payload, strlen(payload), false);
    }

    const char* lastWillFormat PROGMEM = R"({"device":"%s","connected":false})";
    const char* connectionMsgFormat PROGMEM = R"({"device":"%s","connected":true})";
    PangolinMQTT mqtt;
    String topic;
    Ticker reconnectTimer;
} // namespace MQTT
