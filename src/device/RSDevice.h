#pragma once

#include <functional>
#include <stdint.h>
#include <string>

#include <ArduinoJson.h>

#include "Config.h"
#include "MqttInterface.h"

/// Telemetry PVOutput needs from a charge controller.
struct PVOutputData
{
    int16_t energyGenerated = 0; /// Generated energy in Wh
    int16_t energyConsumed = 0; /// Consumed energy in Wh
    float powerGeneration = 0.0f; /// Generated power in W
    float powerConsumption = 0.0f; /// Consumed power in W
    float temperature = 0.0f; /// Battery temperature in °C
    float voltage = 0.0f; /// Battery voltage in V
};

// class Strategy
// {
// public:
//     virtual ~Strategy() = default;
//     virtual std::string doAlgorithm(std::string_view data) const = 0;
// };

class RSDevice
{
public:
    /// @brief Callback definition for data listener
    typedef std::function<void()> Listener;
    // typedef std::function<void(const String& topic, const JsonDocument& data, const bool retain)> MqttPublish;

public:
    virtual ~RSDevice() = default;

    /// @brief Set a listener which gets notified of data updates
    /// @param listener Listener or null
    void setListener(Listener listener) { _listener = listener; }
    /// @brief Read and process device data
    virtual void readAndProcessData() = 0;
    /// @brief Get the value for the given input type
    /// @param type Input type
    /// @return Value or 0
    virtual float getValueForType(const InputType type) const = 0;
    /// @brief Enable or disable the load output of the controller
    /// @param enable True to enable, false to disable load output
    virtual void enableLoad(const bool enable) = 0;
    /// @brief Whether this device can supply the solar and load telemetry PVOutput needs.
    virtual bool supportsPVOutput() const { return false; }
    /// @brief Return the latest PVOutput-compatible controller telemetry.
    virtual PVOutputData getPVOutputData() const { return {}; }
    virtual void updateUI(JsonDocument& json) const = 0;
    virtual void publishHaDiscovery(MqttInterface& publish) const = 0;
    virtual void publishIndividualMqttData(MqttInterface& publish) const = 0;

protected:
    /// @brief Notify the listener
    inline void notifyListener() const
    {
        if (_listener)
        {
            _listener();
        }
    }

private:
    /// @brief Listener callback
    Listener _listener;
};
