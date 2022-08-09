#pragma once

#include <functional>

#include <HardwareSerial.h>
#include <ModbusMaster.h>

class Renogy
{
public:
    ///@brief Contains data retreived from charge controller
    struct Data
    {
        int32_t errorState = 0; /// Controller error state
        uint8_t batteryCharge = 0; /// Battery Charge in % [0-100]
        int8_t batteryTemperature = 0; /// Battery temperature in degrees C
        int8_t chargingState = 0; /// Controller charging state
        int8_t controllerTemperature = 0; /// Controller temperature in degrees C

        float loadVoltage = 0.0f; /// Load output voltage in Volt
        float loadCurrent = 0.0f; /// Load output current in Ampere

        float batteryVoltage = 0.0f; /// Batery voltage in Volt
        float batteryCurrent = 0.0f; /// Battery current in Ampere

        float panelVoltage = 0.0f; /// Solar panel voltage in Volt
        float panelCurrent = 0.0f; /// Solar panel current in Ampere

        bool loadEnabled = false; /// Load output enabled state, true=enabled, false=disabled
    } _data;

    ///@brief Callback definition for data listener
    typedef std::function<void(const Data&)> DataListener;

public:
    ///@brief Construct a new Renogy object
    ///
    ///@param serial Hardware Serial for ModBus communication
    Renogy(HardwareSerial& serial)
    {
        serial.setTimeout(100);
        // Modbus at 9600 baud
        serial.begin(9600);
        // Maybe make configurable with updateBaudrate(baud);

        // Renogy Device ID = 255 (was 1)
        // _modbus.begin(0x01, serial);
        _modbus.begin(0xFF, serial);
        // Maybe make configurable with begin(x, Serial);
        // TODO Maybe need to check each device ID to find correct one

        // D2 = RS485 DE/!RE (direction)
        pinMode(D2, OUTPUT);
        _modbus.preTransmission([]() {
            // delay(100);
            digitalWrite(D2, HIGH);
        });
        _modbus.postTransmission([]() {
            digitalWrite(D2, LOW);
            // delay(100);
        });
    }

    Renogy(Renogy&&) = delete;

    ///@brief Read and process the modbus data
    void readAndProcessData();

    ///@brief Enable or disable the load output of the controller
    ///
    ///@param enable True to enable, false to disable load output
    void enableLoad(const bool enable);

    ///@brief Set a listener which receives @ref Renogy::Data updates
    ///
    ///@param listener Listener or null
    void setListener(DataListener listener);

    void readModel();

private:
    ModbusMaster _modbus;
    DataListener _listener;
    String model = "";
}; // class Renogy
