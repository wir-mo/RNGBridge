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
    OutputControl(RSDevice& device, DeviceConfig& deviceConfig);

    /// @brief Update output states depending on individual OutputConfig
    ///
    /// @param device Device for getting data
    void update(const RSDevice& device);

    /// @brief Control renogy load output
    ///
    /// @param enable True to turn on, false to turn off
    void enableLoad(const bool enable);
    /// @brief Control RNGBridge output 1
    ///
    /// @param enable True to turn on, false to turn off
    void enableOut1(const bool enable);
    /// @brief Control RNGBridge output 2
    ///
    /// @param enable True to turn on, false to turn off
    void enableOut2(const bool enable);
    /// @brief Control RNGBridge output 3
    ///
    /// @param enable True to turn on, false to turn off
    void enableOut3(const bool enable);

private:
    /// @brief Handle output control for a given output
    ///
    /// @param tag Debug tag
    /// @param output Output configuration with setpoints
    /// @param device Device for getting data
    /// @param enable Callback function for turning output on (passing true) or off (passing false)
    void handleOutput(
        const char* tag, OutputConfig& output, const RSDevice& device, std::function<void(const bool)> enable);

private:
    constexpr static const uint8_t PIN_OUTPUT1 = D5; /// pin definition for first output control
    constexpr static const uint8_t PIN_OUTPUT2 = D6; /// pin definition for second output control
    constexpr static const uint8_t PIN_OUTPUT3 = D7; /// pin definition for third output control

    DeviceConfig& deviceConfig; /// Reference to device config including output config

    std::function<void(const bool)> handleLoad; /// callback to control renogy load output
    std::function<void(const bool)> handleOut1; /// callback to control RNGBridge output 1
    std::function<void(const bool)> handleOut2; /// callback to control RNGBridge output 2
    std::function<void(const bool)> handleOut3; /// callback to control RNGBridge output 3
};
