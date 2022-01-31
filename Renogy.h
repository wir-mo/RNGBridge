#pragma once

#include <functional>

#include <HardwareSerial.h>
#include <ModbusMaster.h>

class Renogy
{
public:
    struct Data
    {
        uint8_t batteryCharge = 0;
        int8_t batteryTemperature = 0;
        int8_t chargingState = 0;
        int8_t controllerTemperature = 0;
        int16_t loadPower = 0;
        int16_t panelPower = 0;
        int32_t errorState = 0;

        bool loadEnabled = false;
        float loadVoltage = 0.0f;
        float loadCurrent = 0.0f;
        float batteryVoltage = 0.0f;
        float batteryCurrent = 0.0f;
        float panelVoltage = 0.0f;
        float panelCurrent = 0.0f;
    } _data;

    typedef std::function<void(const Data&)> Listener;

    // class Listener
    // {
    // public:
    //     virtual ~Listener() = default;
    //     virtual void onData(const Data&) = 0;
    // };

public:
    Renogy(HardwareSerial& serial)
    {
        serial.setTimeout(100);
        // Modbus at 9600 baud
        serial.begin(9600);
        // Maybe make configurable with updateBaudrate(baud);

        // Renogy Device ID = 1
        _modbus.begin(1, serial);
        // Maybe make configurable with begin(x, Serial);
        // TODO Maybe need to check each device ID to find correct one
    }

    Renogy(Renogy&&) = delete;

    ///@brief Read and process the modbus data
    void readAndProcessData();

    ///@brief Enable or disable the load output of the controller
    ///
    ///@param enable True to enable, false to disable load output
    void enableLoad(const boolean enable);

    ///@brief Set a listener which receives @ref Renogy::Data updates
    ///
    ///@param listener Listener or null
    void setListener(Listener listener);

private:
    void writeException(const uint8_t code);

private:
    ModbusMaster _modbus;
    Listener _listener;
}; // class Renogy
