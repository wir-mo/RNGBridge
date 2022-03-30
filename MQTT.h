#pragma once

#include <functional>

#include <PubSubClient.h>

#include "Config.h"
#include "Renogy.h"

class Mqtt
{
public:
    typedef std::function<void(const String&)> Listener;

public:
    Mqtt(const MqttConfig& mqttConfig) : mqttConfig(mqttConfig), mqtt(espClient)
    {
        _status = "Enabled";
        mqtt.setServer(mqttConfig.server.c_str(), mqttConfig.port);
    }

    Mqtt(Mqtt&&) = delete;

    void connect();

    void disconnect();

    void loop();

    void updateRenogyStatus(const Renogy::Data& data);

    ///@brief Set a listener which receives status updates
    ///
    ///@param listener Listener or null
    void setListener(Listener listener);

private:
    void publish(const String& payload, uint8_t qos = 0, bool retain = false);
    void publish(const char* payload, uint8_t qos = 0, bool retain = false);
    void publish(const char* topic, const char* payload, uint8_t qos = 0, bool retain = false);

    ///@brief Update the internal status string and notify listener
    ///
    ///@param status New status
    void updateStatus(const String& status);

private:
    static const char* statusFormat PROGMEM;
    static const char* lastWillFormat PROGMEM;
    static const char* connectionMsgFormat PROGMEM;
    const MqttConfig& mqttConfig;
    WiFiClient espClient;
    PubSubClient mqtt;
    Listener _listener;
    String _status;
    uint16_t updateCount = 0; // counter used to determine when to send mqtt status message
}; // class MQTT
