#pragma once

#include <functional>

#include "Config.h"
#include "Observerable.h"

#include "device/RSDevice.h"

/// @brief Current output status
struct OutputStatus
{
    bool out1 = false; /// The current state of output 1 (on=true, off=false)
    bool out2 = false; /// The current state of output 2 (on=true, off=false)
    bool out3 = false; /// The current state of output 3 (on=true, off=false)
};

/// @brief Class for controlling all outputs (Renogy Load output, out1, out2 and out3)
///
class OutputControl : public Observerable<OutputStatus>
{
public:
    /// @brief Construct a new Output Control object
    ///
    /// @param device RS controller
    /// @param deviceConfig Device config including output configs
    OutputControl(RSDevice& device, DeviceConfig& deviceConfig) : deviceConfig(deviceConfig)
    {
        pinMode(PIN_OUTPUT1, OUTPUT);
        pinMode(PIN_OUTPUT2, OUTPUT);
        pinMode(PIN_OUTPUT3, OUTPUT);
        digitalWrite(PIN_OUTPUT1, LOW);
        digitalWrite(PIN_OUTPUT2, LOW);
        digitalWrite(PIN_OUTPUT3, LOW);
        handleLoad = [&](const bool enable) {
            device.enableLoad(enable);
            deviceConfig.load.lastState = enable;
        };
        handleOut1 = [&](const bool enable) {
            digitalWrite(PIN_OUTPUT1, enable);
            deviceConfig.out1.lastState = enable;
            _value.out1 = enable;
            notify(_value);
        };
        handleOut2 = [&](const bool enable) {
            digitalWrite(PIN_OUTPUT2, enable);
            deviceConfig.out2.lastState = enable;
            _value.out2 = enable;
            notify(_value);
        };
        handleOut3 = [&](const bool enable) {
            digitalWrite(PIN_OUTPUT3, enable);
            deviceConfig.out3.lastState = enable;
            _value.out3 = enable;
            notify(_value);
        };
    }

    /// @brief Update output states depending on individual OutputConfig
    ///
    /// @param device Device for getting data
    void update(const RSDevice& device)
    {
        handleOutput("Load", deviceConfig.load, device, handleLoad);
        handleOutput("Out1", deviceConfig.out1, device, handleOut1);
        handleOutput("Out2", deviceConfig.out2, device, handleOut2);
        handleOutput("Out3", deviceConfig.out3, device, handleOut3);
    }

    /// @brief Control renogy load output
    ///
    /// @param enable True to turn on, false to turn off
    void enableLoad(const bool enable) { handleLoad(enable); }
    /// @brief Control RSBridge output 1
    ///
    /// @param enable True to turn on, false to turn off
    void enableOut1(const bool enable) { handleOut1(enable); }
    /// @brief Control RSBridge output 2
    ///
    /// @param enable True to turn on, false to turn off
    void enableOut2(const bool enable) { handleOut2(enable); }
    /// @brief Control RSBridge output 3
    ///
    /// @param enable True to turn on, false to turn off
    void enableOut3(const bool enable) { handleOut3(enable); }

private:
    /// @brief Handle output control for a given output
    ///
    /// @param tag Debug tag
    /// @param output Output configuration with setpoints
    /// @param device Device for getting data
    /// @param enable Callback function for turning output on (passing true) or off (passing false)
    void handleOutput(
        const char* tag, OutputConfig& output, const RSDevice& device, std::function<void(const bool)> enable)
    {
        if (output.inputType == InputType::disabled)
        {
            return;
        }

        const float value = device.getValueForType(output.inputType);

        RS_DEBUGF("[OutputControl][%s] min %.2f, max %.2f, value %.2f\n", tag, output.min, output.max, value);

        if (value >= output.max)
        {
            const bool newState = !output.inverted;
            if (output.lastState != newState)
            {
                // output.lastState = newState;
                enable(newState);
                RS_DEBUGF(
                    "[OutputControl][%s] %.2f>=%.2f turned %s\n", tag, value, output.max, newState ? "on" : "off");
            }
        }
        else if (value < output.min)
        {
            const bool newState = output.inverted;
            if (output.lastState != newState)
            {
                // output.lastState = newState;
                enable(newState);
                RS_DEBUGF("[OutputControl][%s] %.2f<%.2f turned %s\n", tag, value, output.min, newState ? "on" : "off");
            }
        }
    }

private:
    constexpr static const uint8_t PIN_OUTPUT1 = D5; /// pin definition for first output control
    constexpr static const uint8_t PIN_OUTPUT2 = D6; /// pin definition for second output control
    constexpr static const uint8_t PIN_OUTPUT3 = D7; /// pin definition for third output control

    DeviceConfig& deviceConfig; /// Reference to device config including output config

    std::function<void(const bool)> handleLoad; /// callback to control renogy load output
    std::function<void(const bool)> handleOut1; /// callback to control RSBridge output 1
    std::function<void(const bool)> handleOut2; /// callback to control RSBridge output 2
    std::function<void(const bool)> handleOut3; /// callback to control RSBridge output 3
};
