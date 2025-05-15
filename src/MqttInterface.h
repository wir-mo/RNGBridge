#pragma once

class MqttInterface
{
public:
    virtual void publishSub(const char* topic, const char* value, const bool retain = false) = 0;

    /// @brief Publish homeassistant sensor discorvery message
    ///
    /// @param name Sensor name
    /// @param id Unique sensor identifier
    /// @param valueTemplate Template for sensor value
    /// @param icon Optional sensor icon
    virtual void publishSensorDiscovery(
        const String& name, const String& id, const String& valueTemplate, const String& icon = "")
        = 0;

    /// @brief Publish homeassistant extended sensor discorvery message
    ///
    /// @param name Sensor name
    /// @param id Unique sensor identifier
    /// @param deviceClass Sensor device class
    /// @param unit Sensor unit (e.g. `°C`, `%`, `W`, etc.)
    /// @param stateClass Sensor state class
    /// @param valueTemplate Template for sensor value
    /// @param icon Optional sensor icon
    virtual void publishSensorDiscovery(const String& name, const String& id, const String& deviceClass,
        const String& unit, const String& stateClass, const String& valueTemplate, const String& icon = "")
        = 0;

    /// @brief Publish homeassistant switch discorvery message
    ///
    /// @param name Switch name
    /// @param id Unique switch identifier
    /// @param valueTemplate Template for switch state
    /// @param icon Optional switch icon
    virtual void publishSwitchDiscovery(
        const String& name, const String& id, const String& valueTemplate, const String& icon = "")
        = 0;
};