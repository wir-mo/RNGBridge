#pragma once

#include <functional>

#include <PubSubClient.h>

#include "Config.h"
#include "Observerable.h"
#include "OutputControl.h"
#include "Renogy.h"

// Quality Of Service (QOS)
// At most once (0)
// At least once (1)
// Exactly once (2)

class Mqtt : public Observerable<String>
{
public:
    Mqtt(const MqttConfig& mqttConfig, OutputControl& outputs)
        : mqttConfig(mqttConfig), outputs(outputs), mqtt(espClient)
    {
        notify("Enabled");
        // mqtt.setBufferSize(512);
        mqtt.setServer(mqttConfig.server.c_str(), mqttConfig.port);
    }

    Mqtt(Mqtt&&) = delete;

    void connect();

    void disconnect();

    void loop();

    void updateRenogyStatus(const Renogy::Data& data);

private:
    const String getDeviceID() { return String("rngbridge-") + deviceMAC; }
    /// @brief Setup load control via MQTT
    ///
    /// Will subscribe control topics for each output and then register a callback for handling received messages
    void setupLoadControl();

    /// @brief Add the device info object to the given json object
    ///
    /// @param json JSON object
    /// @param deviceID Device identifier
    void addDeviceInfo(JsonDocument& json, const String& deviceID);

    /// @brief Publish homeassistant sensor discorvery message
    ///
    /// @param name Sensor name
    /// @param id Unique sensor identifier
    /// @param valueTemplate Template for sensor value
    /// @param icon Optional sensor icon
    void publishSensorDiscovery(
        const String& name, const String& id, const String& valueTemplate, const String& icon = "");

    /// @brief Publish homeassistant extended sensor discorvery message
    ///
    /// @param name Sensor name
    /// @param id Unique sensor identifier
    /// @param deviceClass Sensor device class
    /// @param unit Sensor unit (e.g. `°C`, `%`, `W`, etc.)
    /// @param stateClass Sensor state class
    /// @param valueTemplate Template for sensor value
    /// @param icon Optional sensor icon
    void publishSensorDiscovery(const String& name, const String& id, const String& deviceClass, const String& unit,
        const String& stateClass, const String& valueTemplate, const String& icon = "");

    /// @brief Publish homeassistant switch discorvery message
    ///
    /// @param name Switch name
    /// @param id Unique switch identifier
    /// @param valueTemplate Template for switch state
    /// @param icon Optional switch icon
    void publishSwitchDiscovery(
        const String& name, const String& id, const String& valueTemplate, const String& icon = "");

    /// @brief Subscribe to a given topic
    ///
    /// @param topic Topic to subscribe to
    void subscribe(const String& topic);
    /// @brief Publish the given JSON document to the topic
    ///
    /// @param topic Topic to publish to
    /// @param json JSON document to publish
    /// @param retain OPtionally reatin the message (true) or not (false)
    /// @return true if the JSON document was published
    /// @return false if not
    bool publishJSON(const String& topic, const JsonDocument& json, const bool retain = false);
    bool publish(const String& payload, bool retain = false);
    bool publish(const char* payload, bool retain = false);
    bool publish(const char* topic, const char* payload, bool retain = false);
    bool publishLarge(const char* topic, const char* payload, bool retain = false)
    {
        if (mqtt.beginPublish(topic, strlen(payload), retain))
        {
            mqtt.write(reinterpret_cast<const uint8_t*>(payload), strlen(payload));
            return mqtt.endPublish();
        }
        return false;
    }

private:
    const MqttConfig& mqttConfig;
    OutputControl& outputs;
    WiFiClient espClient;
    PubSubClient mqtt;
    uint32_t lastUpdate = 0; /// last time in seconds we updated
}; // class MQTT
