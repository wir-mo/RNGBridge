#pragma once

#include <functional>

#include <PubSubClient.h>

#include "Config.h"
#include "Observerable.h"
#include "OutputControl.h"

// Quality Of Service (QOS)
// At most once (0)
// At least once (1)
// Exactly once (2)

class HaDevice
{
public:
    constexpr HaDevice(const char* uniqueId, const char* manufacturer, const char* model, const char* softwareVersion)
        : uniqueId(uniqueId), manufacturer(manufacturer), model(model), softwareVersion(softwareVersion)
    { }

    void setUniqueId(const char* uid) { uniqueId = uid; }

private:
    const char* uniqueId;
    const char* manufacturer;
    const char* model;
    const char* softwareVersion;
    const char* configurationUrl = nullptr;
};

class Topic
{
    constexpr Topic(const char* name, const char* valueTemplate, const char* deviceClass = nullptr,
        const char* unit = nullptr, const char* stateClass = nullptr, const char* icon = nullptr)
        : name(name),
          valueTemplate(valueTemplate),
          deviceClass(deviceClass),
          unit(unit),
          stateClass(stateClass),
          icon(icon)
    { }

private:
    const char* name;
    const char* valueTemplate;
    const char* deviceClass;
    const char* unit;
    const char* stateClass;
    const char* icon;
};

class Mqtt : public MqttInterface, Observerable<String>
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

    void publishData();

    void publishSub(const char* topic, const char* value, const bool retain = false) override
    {
        publish((mqttConfig.topic + topic).c_str(), value, retain);
    }

    /// @brief Publish the given JSON document to the topic
    ///
    /// @param topic Topic to publish to
    /// @param json JSON document to publish
    /// @param retain Optionally reatin the message (true) or not (false)
    /// @return true if the JSON document was published
    /// @return false if not
    bool publishJSON(const String& topic, const JsonDocument& json, const bool retain = false);
    bool publish(const String& payload, bool retain = false);
    bool publish(const char* payload, bool retain = false);
    bool publish(const char* topic, const char* payload, bool retain = false);
    /// @brief Publish a large amount of data
    /// @param topic Topic to publish to
    /// @param payload Data to publish
    /// @param retain Optionally reatin the message (true) or not (false)
    /// @return true if the data was published, false if not
    bool publishLarge(const char* topic, const char* payload, bool retain = false)
    {
        if (mqtt.beginPublish(topic, strlen(payload), retain))
        {
            mqtt.write(reinterpret_cast<const uint8_t*>(payload), strlen(payload));
            return mqtt.endPublish();
        }
        return false;
    }

    void publishSensorDiscovery(
        const String& name, const String& id, const String& valueTemplate, const String& icon = "") override;

    void publishSensorDiscovery(const String& name, const String& id, const String& deviceClass, const String& unit,
        const String& stateClass, const String& valueTemplate, const String& icon = "") override;

    void publishSwitchDiscovery(
        const String& name, const String& id, const String& valueTemplate, const String& icon = "") override;

    bool splitData() const { return mqttConfig.split; }

private:
    const String getDeviceID() { return String("rsbridge-") + deviceMAC; }
    /// @brief Setup load control via MQTT
    ///
    /// Will subscribe control topics for each output and then register a callback for handling received messages
    void setupLoadControl();

    /// @brief Add the device info object to the given json object
    ///
    /// @param json JSON object
    /// @param deviceID Device identifier
    void addDeviceInfo(JsonDocument& json, const String& deviceID);

    /// @brief Subscribe to a given topic
    ///
    /// @param topic Topic to subscribe to
    void subscribe(const String& topic);

private:
    const MqttConfig& mqttConfig;
    OutputControl& outputs;
    WiFiClient espClient;
    PubSubClient mqtt;
    uint32_t lastUpdate = 0; /// last time in seconds we updated
}; // class MQTT
