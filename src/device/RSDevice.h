#pragma once

#include <functional>
#include <string>

#include <ArduinoJson.h>

#include "Config.h"
#include "MqttInterface.h"

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
