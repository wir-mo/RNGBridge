#pragma once

#include <functional>

#include <HardwareSerial.h>
#include <ModbusMaster.h>

// 5001 - 5016 - cell voltage * 0.1 in V
// 5018 - 5034 - cell temperature * 0.1 in °C
// 5035 - bms temperature * 0.1 in °C
// 5037 - 5038 - ambient temperature * 0.1 in °C
// 5040 - 5041 - heater temperature * 0.1 in °C
// 5042 - current * 0.01 in A
// 5043 - voltage * 0.1 in V
// 5044 - 5045 - remaining capacity * 0.001 in Ah
// 5046 - 5047 - total capacity * 0.001 in Ah
// 5048 - cycle number
// 5049 - charge voltage limit * 0.1 in V
// 5050 - discharge voltage limit * 0.1 in V
// 5050 - charge current limit * 0.01 in A
// 5050 - discharge current limit * 0.01 in A

class Renogy
{
public:
    /// @brief Contains data retreived from charge controller
    struct Data
    {
        uint16_t cycles = 0; /// cycle count
        uint16_t cellCount = 0; /// amount of cells
        float cellVoltage[16] = {0}; /// cell voltages
        float cellTemperature[16] = {0}; /// cell temperatures
        float ambientTemperature[2] = {0}; /// ambient temperatures
        float heaterTemperature[2] = {0}; /// heater temperatures
        float bmsTemperature = 0; /// bms temperature
        float current = 0; /// current
        float voltage = 0; /// voltage
        float remaining = 0; /// remaining capacity
        float total = 0; /// total capacity
        float chargeVoltageLimit = 0; /// charge voltage limit
        float dischargeVoltageLimit = 0; /// discharge voltage limit
        float chargeCurrentLimit = 0; /// charge current limit
        float dischargeCurrentLimit = 0; /// discharge current limit
    } _data;

    /// @brief Callback definition for data listener
    typedef std::function<void(const Data&)> DataListener;

public:
    /// @brief Construct a new Renogy object
    /// @param serial Hardware Serial for ModBus communication
    /// @param address Modbus device address
    Renogy(HardwareSerial& serial, const uint8_t address)
    {
        serial.setTimeout(100);
        // Modbus at 9600 baud
        serial.begin(9600);
        // Maybe make configurable with updateBaudrate(baud);

        _modbus.begin(address, serial);

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

    /// @brief Read and process the modbus data
    void readAndProcessData();

    /// @brief Enable or disable the load output of the controller
    ///
    /// @param enable True to enable, false to disable load output
    void enableLoad(const bool enable);

    /// @brief Set a listener which receives @ref Renogy::Data updates
    ///
    /// @param listener Listener or null
    void setListener(DataListener listener);

    void readModel();

private:
    ModbusMaster _modbus;
    DataListener _listener;
    String model = "";
}; // class Renogy
