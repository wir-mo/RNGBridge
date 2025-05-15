#pragma once

#include <functional>

#include <HardwareSerial.h>
#include <ModbusMaster.h>

#include "Constants.h"
#include "MQTT.h"
#include "Modbus.h"
#include "PVOutput.h"
#include "RSDevice.h"

class RenogyBattery : public RSDevice
{
public:
    /// @brief Contains _data retreived from charge controller
    struct _data
    {
        uint16_t cycles = 0; /// cycle count
        uint16_t cellCount = 0; /// amount of cells
        float cellVoltage[16] = {0}; /// cell voltages in V
        float cellTemperature[16] = {0}; /// cell temperatures in °C
        float ambientTemperature[2] = {0}; /// ambient temperatures in °C
        float heaterTemperature[2] = {0}; /// heater temperatures in °C
        float bmsTemperature = 0; /// bms temperature in °C
        float current = 0; /// current in A
        float voltage = 0; /// voltage in V
        float remaining = 0; /// remaining capacity in Ah
        float total = 0; /// total capacity in Ah
        float chargeVoltageLimit = 0; /// charge voltage limit in V
        float dischargeVoltageLimit = 0; /// discharge voltage limit in V
        float chargeCurrentLimit = 0; /// charge current limit in A
        float dischargeCurrentLimit = 0; /// discharge current limit in A
    } _data;

public:
    /// @brief Construct a new RenogyBattery object
    /// @param serial Hardware Serial for ModBus communication
    /// @param address Modbus device address
    RenogyBattery(HardwareSerial& serial, const uint8_t address)
    {
        serial.setTimeout(100);
        // Modbus at 9600 baud
        serial.begin(9600);
        // Maybe make configurable with updateBaudrate(baud);

        _modbus.begin(address, serial);

        // D8 = RS485 DE/!RE (direction)
        pinMode(D8, OUTPUT);
        _modbus.preTransmission([]() {
            // delay(100);
            digitalWrite(D8, HIGH);
        });
        _modbus.postTransmission([]() {
            digitalWrite(D8, LOW);
            // delay(100);
        });
    }

    virtual ~RenogyBattery() override = default;

    RenogyBattery(RenogyBattery&&) = delete;

    void readAndProcessData() override
    {
        // Read 34 registers starting at 5000
        _modbus.clearResponseBuffer();

        uint8_t result = _modbus.readHoldingRegisters(5000, 34);
        if (result != _modbus.ku8MBSuccess)
        {
            RNG_DEBUGF("[RenogyBattery] Could not read registers (5000): %s (0x%02X)\n", ModBus::resultToString(result),
                result);
            return;
        }

        // RNG_DEBUGF("Data dump: 0x");
        // for (uint8_t i = 0; i < 52; ++i)
        // {
        //     RNG_DEBUGF("%04" PRIx16, _modbus.getResponseBuffer(i));
        // }
        // RNG_DEBUGLN();

        _data.cellCount = ModBus::readUInt16BE(_modbus, 0);
        _data.cellVoltage[0] = 0.1f * ModBus::readUInt16BE(_modbus, 1);
        _data.cellVoltage[1] = 0.1f * ModBus::readUInt16BE(_modbus, 2);
        _data.cellVoltage[2] = 0.1f * ModBus::readUInt16BE(_modbus, 3);
        _data.cellVoltage[3] = 0.1f * ModBus::readUInt16BE(_modbus, 4);
        _data.cellVoltage[4] = 0.1f * ModBus::readUInt16BE(_modbus, 5);
        _data.cellVoltage[5] = 0.1f * ModBus::readUInt16BE(_modbus, 6);
        _data.cellVoltage[6] = 0.1f * ModBus::readUInt16BE(_modbus, 7);
        _data.cellVoltage[7] = 0.1f * ModBus::readUInt16BE(_modbus, 8);
        _data.cellVoltage[8] = 0.1f * ModBus::readUInt16BE(_modbus, 9);
        _data.cellVoltage[9] = 0.1f * ModBus::readUInt16BE(_modbus, 10);
        _data.cellVoltage[10] = 0.1f * ModBus::readUInt16BE(_modbus, 11);
        _data.cellVoltage[11] = 0.1f * ModBus::readUInt16BE(_modbus, 12);
        _data.cellVoltage[12] = 0.1f * ModBus::readUInt16BE(_modbus, 13);
        _data.cellVoltage[13] = 0.1f * ModBus::readUInt16BE(_modbus, 14);
        _data.cellVoltage[14] = 0.1f * ModBus::readUInt16BE(_modbus, 15);
        _data.cellVoltage[15] = 0.1f * ModBus::readUInt16BE(_modbus, 16);

        _data.cellTemperature[0] = 0.1f * ModBus::readUInt16BE(_modbus, 18);
        _data.cellTemperature[1] = 0.1f * ModBus::readUInt16BE(_modbus, 19);
        _data.cellTemperature[2] = 0.1f * ModBus::readUInt16BE(_modbus, 20);
        _data.cellTemperature[3] = 0.1f * ModBus::readUInt16BE(_modbus, 21);
        _data.cellTemperature[4] = 0.1f * ModBus::readUInt16BE(_modbus, 22);
        _data.cellTemperature[5] = 0.1f * ModBus::readUInt16BE(_modbus, 23);
        _data.cellTemperature[6] = 0.1f * ModBus::readUInt16BE(_modbus, 24);
        _data.cellTemperature[7] = 0.1f * ModBus::readUInt16BE(_modbus, 25);
        _data.cellTemperature[8] = 0.1f * ModBus::readUInt16BE(_modbus, 26);
        _data.cellTemperature[9] = 0.1f * ModBus::readUInt16BE(_modbus, 27);
        _data.cellTemperature[10] = 0.1f * ModBus::readUInt16BE(_modbus, 28);
        _data.cellTemperature[11] = 0.1f * ModBus::readUInt16BE(_modbus, 29);
        _data.cellTemperature[12] = 0.1f * ModBus::readUInt16BE(_modbus, 30);
        _data.cellTemperature[13] = 0.1f * ModBus::readUInt16BE(_modbus, 31);
        _data.cellTemperature[14] = 0.1f * ModBus::readUInt16BE(_modbus, 32);
        _data.cellTemperature[15] = 0.1f * ModBus::readUInt16BE(_modbus, 33);

        delay(100);
        // Read another 18 registers starting at 5035
        result = _modbus.readHoldingRegisters(5035, 18);
        if (result != _modbus.ku8MBSuccess)
        {
            RNG_DEBUGF("[RenogyBattery] Could not read registers (5035): %s (0x%02X)\n", ModBus::resultToString(result),
                result);
            return;
        }
        _data.bmsTemperature = 0.1f * ModBus::readUInt16BE(_modbus, 0);
        // # of ambient temperatures ModBus::readUInt16BE(_modbus, 1);
        _data.ambientTemperature[0] = 0.1f * ModBus::readUInt16BE(_modbus, 2);
        _data.ambientTemperature[1] = 0.1f * ModBus::readUInt16BE(_modbus, 3);

        // # of heater temperatures ModBus::readUInt16BE(_modbus, 4);
        _data.heaterTemperature[0] = 0.1f * ModBus::readUInt16BE(_modbus, 5);
        _data.heaterTemperature[1] = 0.1f * ModBus::readUInt16BE(_modbus, 6);

        _data.current = 0.01f * ModBus::readInt16BE(_modbus, 7);
        _data.voltage = 0.1f * ModBus::readUInt16BE(_modbus, 8);

        _data.remaining = 0.001f * ModBus::readUInt32BE(_modbus, 9);
        _data.total = 0.001f * ModBus::readUInt32BE(_modbus, 11);

        _data.cycles = ModBus::readUInt16BE(_modbus, 13);

        _data.chargeVoltageLimit = 0.1f * ModBus::readUInt16BE(_modbus, 14);
        _data.dischargeVoltageLimit = 0.1f * ModBus::readUInt16BE(_modbus, 15);

        _data.chargeCurrentLimit = 0.01f * ModBus::readUInt16BE(_modbus, 16);
        _data.dischargeCurrentLimit = 0.01f * ModBus::readUInt16BE(_modbus, 17);

        // All data was read so notify the listener
        notifyListener();

        // if (model.isEmpty())
        // {
        //     readModel();
        // }
    }

    void enableLoad(const bool enable) override
    {
        const uint8_t result = _modbus.writeSingleRegister(0x010A, enable ? 0x01 : 0x00);
        if (result != _modbus.ku8MBSuccess)
        {
            RNG_DEBUGF("[RenogyBattery] Could not turn load %s: %s (0x%02X)\n", enable ? "on" : "off",
                ModBus::resultToString(result), result);
        }
    }

    void updateUI(JsonDocument& json) const override
    {
        switch (_data.cellCount)
        {
        case 16: {
            auto cell = json["c16"];
            cell["te"] = _data.cellTemperature[15];
            cell["vo"] = _data.cellVoltage[15];
            // fall through
        }
        case 15: {
            auto cell = json["c15"];
            cell["te"] = _data.cellTemperature[14];
            cell["vo"] = _data.cellVoltage[14];
            // fall through
        }
        case 14: {
            auto cell = json["c14"];
            cell["te"] = _data.cellTemperature[13];
            cell["vo"] = _data.cellVoltage[13];
            // fall through
        }
        case 13: {
            auto cell = json["c13"];
            cell["te"] = _data.cellTemperature[12];
            cell["vo"] = _data.cellVoltage[12];
            // fall through
        }
        case 12: {
            auto cell = json["c12"];
            cell["te"] = _data.cellTemperature[11];
            cell["vo"] = _data.cellVoltage[11];
            // fall through
        }
        case 11: {
            auto cell = json["c11"];
            cell["te"] = _data.cellTemperature[10];
            cell["vo"] = _data.cellVoltage[10];
            // fall through
        }
        case 10: {
            auto cell = json["c10"];
            cell["te"] = _data.cellTemperature[9];
            cell["vo"] = _data.cellVoltage[9];
            // fall through
        }
        case 9: {
            auto cell = json["c9"];
            cell["te"] = _data.cellTemperature[8];
            cell["vo"] = _data.cellVoltage[8];
            // fall through
        }
        case 8: {
            auto cell = json["c8"];
            cell["te"] = _data.cellTemperature[7];
            cell["vo"] = _data.cellVoltage[7];
            // fall through
        }
        case 7: {
            auto cell = json["c7"];
            cell["te"] = _data.cellTemperature[6];
            cell["vo"] = _data.cellVoltage[6];
            // fall through
        }
        case 6: {
            auto cell = json["c6"];
            cell["te"] = _data.cellTemperature[5];
            cell["vo"] = _data.cellVoltage[5];
            // fall through
        }
        case 5: {
            auto cell = json["c5"];
            cell["te"] = _data.cellTemperature[4];
            cell["vo"] = _data.cellVoltage[4];
            // fall through
        }
        case 4: {
            auto cell = json["c4"];
            cell["te"] = _data.cellTemperature[3];
            cell["vo"] = _data.cellVoltage[3];
            // fall through
        }
        case 3: {
            auto cell = json["c3"];
            cell["te"] = _data.cellTemperature[2];
            cell["vo"] = _data.cellVoltage[2];
            // fall through
        }
        case 2: {
            auto cell = json["c2"];
            cell["te"] = _data.cellTemperature[1];
            cell["vo"] = _data.cellVoltage[1];
            // fall through
        }
        case 1: {
            auto cell = json["c1"];
            cell["te"] = _data.cellTemperature[0];
            cell["vo"] = _data.cellVoltage[0];
            // fall through
        }
        }

        json["a1te"] = _data.ambientTemperature[0];
        json["a2te"] = _data.ambientTemperature[2];

        json["h1te"] = _data.heaterTemperature[0];
        json["h2te"] = _data.heaterTemperature[2];

        json["bmste"] = _data.bmsTemperature;

        json["cy"] = _data.cycles;

        json["cu"] = _data.current;
        json["vo"] = _data.voltage;

        json["rem"] = _data.remaining;
        json["tot"] = _data.total;

        auto chargeLimit = json["chlim"];
        chargeLimit["vo"] = _data.chargeVoltageLimit;
        chargeLimit["cu"] = _data.chargeCurrentLimit;
        auto dischargeLimit = json["dchlim"];
        dischargeLimit["vo"] = _data.dischargeVoltageLimit;
        dischargeLimit["cu"] = _data.dischargeCurrentLimit;
    }

    void publishHaDiscovery(MqttInterface& mqtt) const override
    {
        // Battery related
        mqtt.publishSensorDiscovery("Cell 1 Voltage", "c1v", "voltage", "V", "measurement", "{{value_json.c1.vo}}");
        mqtt.publishSensorDiscovery(
            "Cell 1 Temperature", "c1t", "temperature", "°C", "measurement", "{{value_json.c1.te}}");
        mqtt.publishSensorDiscovery("Cell 2 Voltage", "c2v", "voltage", "V", "measurement", "{{value_json.c2.vo}}");
        mqtt.publishSensorDiscovery(
            "Cell 2 Temperature", "c2t", "temperature", "°C", "measurement", "{{value_json.c2.te}}");
        mqtt.publishSensorDiscovery("Cell 3 Voltage", "c3v", "voltage", "V", "measurement", "{{value_json.c3.vo}}");
        mqtt.publishSensorDiscovery(
            "Cell 3 Temperature", "c3t", "temperature", "°C", "measurement", "{{value_json.c3.te}}");
        mqtt.publishSensorDiscovery("Cell 4 Voltage", "c4v", "voltage", "V", "measurement", "{{value_json.c4.vo}}");
        mqtt.publishSensorDiscovery(
            "Cell 4 Temperature", "c4t", "temperature", "°C", "measurement", "{{value_json.c4.te}}");
        mqtt.publishSensorDiscovery("Cell 5 Voltage", "c5v", "voltage", "V", "measurement", "{{value_json.c5.vo}}");
        mqtt.publishSensorDiscovery(
            "Cell 5 Temperature", "c5t", "temperature", "°C", "measurement", "{{value_json.c5.te}}");
        mqtt.publishSensorDiscovery("Cell 6 Voltage", "c6v", "voltage", "V", "measurement", "{{value_json.c6.vo}}");
        mqtt.publishSensorDiscovery(
            "Cell 6 Temperature", "c6t", "temperature", "°C", "measurement", "{{value_json.c6.te}}");
        mqtt.publishSensorDiscovery("Cell 7 Voltage", "c7v", "voltage", "V", "measurement", "{{value_json.c7.vo}}");
        mqtt.publishSensorDiscovery(
            "Cell 7 Temperature", "c7t", "temperature", "°C", "measurement", "{{value_json.c7.te}}");
        mqtt.publishSensorDiscovery("Cell 8 Voltage", "c8v", "voltage", "V", "measurement", "{{value_json.c8.vo}}");
        mqtt.publishSensorDiscovery(
            "Cell 8 Temperature", "c8t", "temperature", "°C", "measurement", "{{value_json.c8.te}}");
        mqtt.publishSensorDiscovery("Cell 9 Voltage", "c9v", "voltage", "V", "measurement", "{{value_json.c9.vo}}");
        mqtt.publishSensorDiscovery(
            "Cell 9 Temperature", "c9t", "temperature", "°C", "measurement", "{{value_json.c9.te}}");
        mqtt.publishSensorDiscovery("Cell 10 Voltage", "c10v", "voltage", "V", "measurement", "{{value_json.c10.vo}}");
        mqtt.publishSensorDiscovery(
            "Cell 10 Temperature", "c10t", "temperature", "°C", "measurement", "{{value_json.c10.te}}");
        mqtt.publishSensorDiscovery("Cell 11 Voltage", "c11v", "voltage", "V", "measurement", "{{value_json.c11.vo}}");
        mqtt.publishSensorDiscovery(
            "Cell 11 Temperature", "c11t", "temperature", "°C", "measurement", "{{value_json.c11.te}}");
        mqtt.publishSensorDiscovery("Cell 12 Voltage", "c12v", "voltage", "V", "measurement", "{{value_json.c12.vo}}");
        mqtt.publishSensorDiscovery(
            "Cell 12 Temperature", "c12t", "temperature", "°C", "measurement", "{{value_json.c12.te}}");
        mqtt.publishSensorDiscovery("Cell 13 Voltage", "c13v", "voltage", "V", "measurement", "{{value_json.c13.vo}}");
        mqtt.publishSensorDiscovery(
            "Cell 13 Temperature", "c13t", "temperature", "°C", "measurement", "{{value_json.c13.te}}");
        mqtt.publishSensorDiscovery("Cell 14 Voltage", "c14v", "voltage", "V", "measurement", "{{value_json.c14.vo}}");
        mqtt.publishSensorDiscovery(
            "Cell 14 Temperature", "c14t", "temperature", "°C", "measurement", "{{value_json.c14.te}}");
        mqtt.publishSensorDiscovery("Cell 15 Voltage", "c15v", "voltage", "V", "measurement", "{{value_json.c15.vo}}");
        mqtt.publishSensorDiscovery(
            "Cell 15 Temperature", "c15t", "temperature", "°C", "measurement", "{{value_json.c15.te}}");
        mqtt.publishSensorDiscovery("Cell 16 Voltage", "c16v", "voltage", "V", "measurement", "{{value_json.c16.vo}}");
        mqtt.publishSensorDiscovery(
            "Cell 16 Temperature", "c16t", "temperature", "°C", "measurement", "{{value_json.c16.te}}");

        mqtt.publishSensorDiscovery(
            "Ambient Temperature 1", "a1te", "temperature", "°C", "measurement", "{{value_json.a1te}}");
        mqtt.publishSensorDiscovery(
            "Ambient Temperature 2", "a2te", "temperature", "°C", "measurement", "{{value_json.a2te}}");

        mqtt.publishSensorDiscovery(
            "Heater 1 Temperature", "h1te", "temperature", "°C", "measurement", "{{value_json.h1te}}");
        mqtt.publishSensorDiscovery(
            "Heater 2 Temperature", "h2te", "temperature", "°C", "measurement", "{{value_json.h2te}}");

        mqtt.publishSensorDiscovery(
            "BMS Temperature", "bmste", "temperature", "°C", "measurement", "{{value_json.bmste}}");

        mqtt.publishSensorDiscovery("Cycles", "cy", "none", "cycles", "measurement", "{{value_json.cy}}");

        mqtt.publishSensorDiscovery("Current", "cu", "current", "A", "measurement", "{{value_json.cu}}");
        mqtt.publishSensorDiscovery("Voltage", "vo", "voltage", "V", "measurement", "{{value_json.vo}}");

        mqtt.publishSensorDiscovery("Remaining", "rem", "energy", "Wh", "measurement", "{{value_json.rem}}");
        mqtt.publishSensorDiscovery("Total", "tot", "energy", "Wh", "measurement", "{{value_json.tot}}");

        mqtt.publishSensorDiscovery(
            "Charge Voltage Limit", "chlimvo", "voltage", "V", "measurement", "{{value_json.chlim.vo}}");
        mqtt.publishSensorDiscovery(
            "Charge Current Limit", "chlimcu", "current", "A", "measurement", "{{value_json.chlim.cu}}");

        mqtt.publishSensorDiscovery(
            "Discharge Voltage Limit", "dchlimvo", "voltage", "V", "measurement", "{{value_json.dchlim.vo}}");
        mqtt.publishSensorDiscovery(
            "Discharge Current Limit", "dchlimcu", "current", "A", "measurement", "{{value_json.dchlim.cu}}");

        // Telemetry
        mqtt.publishSensorDiscovery("RSSI", "rssi", "signal_strength", "dBm", "measurement", "{{value_json.rssi}}");

        // Output
        // TODO command_topic
        mqtt.publishSwitchDiscovery(
            "Out 1", "o1", "{{'true' if value_json.o.o1 else 'false'}}", "mdi:numeric-1-box-outline");
        mqtt.publishSwitchDiscovery(
            "Out 2", "o2", "{{'true' if value_json.o.o2 else 'false'}}", "mdi:numeric-2-box-outline");
        mqtt.publishSwitchDiscovery(
            "Out 3", "o3", "{{'true' if value_json.o.o3 else 'false'}}", "mdi:numeric-3-box-outline");
    }

    void publishIndividualMqttData(MqttInterface& mqtt) const override
    {
        mqtt.publishSub("/cell/1/v", String(_data.cellVoltage[0]).c_str(), false);
        mqtt.publishSub("/cell/1/t", String(_data.cellTemperature[0]).c_str(), false);
        mqtt.publishSub("/cell/2/v", String(_data.cellVoltage[1]).c_str(), false);
        mqtt.publishSub("/cell/2/t", String(_data.cellTemperature[1]).c_str(), false);
        mqtt.publishSub("/cell/3/v", String(_data.cellVoltage[2]).c_str(), false);
        mqtt.publishSub("/cell/3/t", String(_data.cellTemperature[2]).c_str(), false);
        mqtt.publishSub("/cell/4/v", String(_data.cellVoltage[3]).c_str(), false);
        mqtt.publishSub("/cell/4/t", String(_data.cellTemperature[3]).c_str(), false);
        mqtt.publishSub("/cell/5/v", String(_data.cellVoltage[4]).c_str(), false);
        mqtt.publishSub("/cell/5/t", String(_data.cellTemperature[4]).c_str(), false);
        mqtt.publishSub("/cell/6/v", String(_data.cellVoltage[5]).c_str(), false);
        mqtt.publishSub("/cell/6/t", String(_data.cellTemperature[5]).c_str(), false);
        mqtt.publishSub("/cell/7/v", String(_data.cellVoltage[6]).c_str(), false);
        mqtt.publishSub("/cell/7/t", String(_data.cellTemperature[6]).c_str(), false);
        mqtt.publishSub("/cell/8/v", String(_data.cellVoltage[7]).c_str(), false);
        mqtt.publishSub("/cell/8/t", String(_data.cellTemperature[7]).c_str(), false);
        mqtt.publishSub("/cell/9/v", String(_data.cellVoltage[8]).c_str(), false);
        mqtt.publishSub("/cell/9/t", String(_data.cellTemperature[8]).c_str(), false);
        mqtt.publishSub("/cell/10/v", String(_data.cellVoltage[9]).c_str(), false);
        mqtt.publishSub("/cell/10/t", String(_data.cellTemperature[9]).c_str(), false);
        mqtt.publishSub("/cell/11/v", String(_data.cellVoltage[10]).c_str(), false);
        mqtt.publishSub("/cell/11/t", String(_data.cellTemperature[10]).c_str(), false);
        mqtt.publishSub("/cell/12/v", String(_data.cellVoltage[11]).c_str(), false);
        mqtt.publishSub("/cell/12/t", String(_data.cellTemperature[11]).c_str(), false);
        mqtt.publishSub("/cell/13/v", String(_data.cellVoltage[12]).c_str(), false);
        mqtt.publishSub("/cell/13/t", String(_data.cellTemperature[12]).c_str(), false);
        mqtt.publishSub("/cell/14/v", String(_data.cellVoltage[13]).c_str(), false);
        mqtt.publishSub("/cell/14/t", String(_data.cellTemperature[13]).c_str(), false);
        mqtt.publishSub("/cell/15/v", String(_data.cellVoltage[14]).c_str(), false);
        mqtt.publishSub("/cell/15/t", String(_data.cellTemperature[14]).c_str(), false);
        mqtt.publishSub("/cell/16/v", String(_data.cellVoltage[15]).c_str(), false);
        mqtt.publishSub("/cell/16/t", String(_data.cellTemperature[15]).c_str(), false);
    }

    float getValueForType(const InputType type) const override
    {
        switch (type)
        {
        case InputType::bsoc:
            return uint8_t(_data.remaining / _data.total) * 100;
        case InputType::bvoltage:
            return _data.voltage;
        case InputType::btemp:
            return _data.cellTemperature[0];
        // case InputType::pvoltage:
        //     return _data.panelVoltage;
        // case InputType::pcurrent:
        //     return _data.panelCurrent;
        case InputType::ctemp:
            return _data.bmsTemperature;

        // case InputType::disabled:
        default:
            return 0;
        }
    }

private:
    void readModel()
    {
        _modbus.clearResponseBuffer();
        const uint8_t result = _modbus.readHoldingRegisters(0x000C, 19);

        if (result == _modbus.ku8MBSuccess)
        {
            model = ModBus::readString(_modbus, 0, 8);
            RNG_DEBUGF("[RenogyBattery] Model: %s, SWV: %d, HWV: %d, S#: %d addr: %d, ProtV: %d\n", model.c_str(),
                ModBus::readInt32BE(_modbus, 8), ModBus::readInt32BE(_modbus, 10), ModBus::readInt32BE(_modbus, 12),
                ModBus::readInt8Lower(_modbus, 14), ModBus::readInt32BE(_modbus, 15));
        }
        else
        {
            RNG_DEBUGF("[RenogyBattery] Could not read registers: %d\n", result);
        }
    }

private:
    ModbusMaster _modbus;
    String model = "";
}; // class RenogyBattery

// 0x0100 (2) 00 - Battery capacity SOC (state of charge)
// 0x0101 (2) 01 - Battery voltage * 0.1
// 0x0102 (2) 02 - Charging current to battery * 0.01
// 0x0103 (2) 03 - Upper byte controller temperature bit 7 sign, bits 0 - 6 value
//            03 - Lower byte battery temperature bit 7 sign, bits 0 - 6 value
// 0x0104 (2) 04 - Street light (load) voltage  * 0.1
// 0x0105 (2) 05 - Street light (load) current * 0.01
// 0x0106 (2) 06 - Street light (load) power actual value
// 0x0107 (2) 07 - Solar panel voltage  * 0.1
// 0x0108 (2) 08 - Solar panel current * 0.01
// 0x0109 (2) 09 - Charging Power actual value
// 0x010A (2) 10 - light on/off command (write only 0 for off, 1 for on)
// 0x010B (2) 11 - Battery min voltage of current day * 0.1
// 0x010C (2) 12 - Battery max voltage of current day * 0.1
// 0x010D (2) 13 - max charging current of current day * 0.01
// 0x010E (2) 14 - max discharging current of current day * 0.01
// 0x010F (2) 15 - max charging power of the current day actual value
// 0x0110 (2) 16 - max discharging power of the current day actual value
// 0x0111 (2) 17 - charging amp hours of the current day actual value
// 0x0112 (2) 18 - discharging amp hours of the current day actual value
// 0x0113 (2) 19 - power generation of the current day Wh
// 0x0114 (2) 20 - power consumption of the current day Wh
//
// Historical Information
//
// 0x0115 (2) 21 - total number of operating days
// 0x0116 (2) 22 - total number of battery over-discharges
// 0x0117 (2) 23 - total number of battery full discharges
// 0x0118 (4) 24 - total charging amp-hrs of the battery actual value
// 0x011A (4) 26 - total discharging amp-hrs of the battery actual value
// 0x011C (4) 28 - cumulative power generation Wh
// 0x011E (4) 30 - cumulative power consumption Wh
//
// 0x0120 (2) 32 - charging state in 8 lower bits.
//            00H: charging deactivated
//            01H: charging activated
//            02H: mppt charging mode
//            03H: equalizing charging mode
//            04H: boost charging mode
//            05H: floating charging mode
//            06H: current limiting (overpower)
//
//            - upper 8 bits are street light (load output) status and brightness.
//            00H - 06H: brightness value
//            07H: light on (1) or off (0)
//
// 0x0121 (4) 33 - controller fault and warning information
//            - 32 bit value of flags
//
//            E16 B31: Fan alarm
//            E15 B30: Charge MOS short circuit
//            E14 B29: Anti-reverse MOS short circuit
//            E13 B28: Solar panel reversly connected
//            E12 B27: Solar panel working point over-voltage
//            E11 B26: Solar panel counter current
//            E10 B25: Photovoltaic input side over voltage
//            E09 B24: Photovoltaic input side short circuit
//            E08 B23: Photovoltaic input overpower
//            E07 B22: Ambient temperature too high
//            E06 B21: Controller temperature too high
//            E05 B20: Load overpower or load over-current
//            E04 B19: Load short circuit
//            E03 B18: Battery under-voltage warning
//            E02 B17: Battery over-voltage
//            E01 B16: battery over-discharge
//                B0-B15: Reserved
//

// Smart Battery
// 5000                  - cell count
// 5001 - 5016 ( 0 - 15) - cell voltage * 0.1 in V
// 5018 - 5033 (17 - 32) - cell temperature * 0.1 in °C
// 5035        (34)      - bms temperature * 0.1 in °C
// 5037 - 5038 (36 - 37) - ambient temperature * 0.1 in °C
// 5040 - 5041 (39 - 40) - heater temperature * 0.1 in °C
// 5042        (41)      - current * 0.01 in A
// 5043        (42)      - voltage * 0.1 in V
// 5044 - 5045 (43 - 44) - remaining capacity * 0.001 in Ah
// 5046 - 5047 (45 - 46) - total capacity * 0.001 in Ah
// 5048        (47)      - cycle number
// 5049        (48)      - charge voltage limit * 0.1 in V
// 5050        (49)      - discharge voltage limit * 0.1 in V
// 5051        (50)      - charge current limit * 0.01 in A
// 5052        (51)      - discharge current limit * 0.01 in A
// SOC = remaining capacity / total capacity

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