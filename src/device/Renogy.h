#pragma once

#include <functional>

#include <HardwareSerial.h>
#include <ModbusMaster.h>

#include "Constants.h"
#include "MQTT.h"
#include "Modbus.h"
#include "PVOutput.h"
#include "RSDevice.h"

class Renogy : public RSDevice
{
public:
    /// @brief Contains _data retreived from charge controller
    struct _data
    {
        int32_t errorState = 0; /// Controller error state
        int32_t total = 0; /// Total power generation in Wh
        int16_t generation = 0; /// Power generation in Wh
        int16_t consumption = 0; /// Power consumption in Wh
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

    virtual ~Renogy() override = default;

    Renogy(Renogy&&) = delete;

    void readAndProcessData() override
    {
        // Read 34 registers starting at 0x0100)
        _modbus.clearResponseBuffer();
        const uint8_t result = _modbus.readHoldingRegisters(0x0100, 34);

        if (result != _modbus.ku8MBSuccess)
        {
            RS_DEBUGF("[Renogy] Could not read registers: %s (0x%02X)\n", ModBus::resultToString(result), result);
            return;
        }

        _data.batteryCharge = ModBus::readInt16BE(_modbus, 0);
        _data.batteryVoltage = 0.1f * ModBus::readInt16BE(_modbus, 1);
        _data.batteryCurrent = 0.01f * ModBus::readInt16BE(_modbus, 2);
        _data.controllerTemperature = ModBus::readInt8Upper(_modbus, 3);
        if (_data.controllerTemperature & 0x80)
        {
            _data.controllerTemperature = -(_data.controllerTemperature & 0x7f);
        }
        _data.batteryTemperature = ModBus::readInt8Lower(_modbus, 3);
        if (_data.batteryTemperature & 0x80)
        {
            _data.batteryTemperature = -(_data.batteryTemperature & 0x7f);
        }

        _data.loadVoltage = 0.1f * ModBus::readInt16BE(_modbus, 4);
        _data.loadCurrent = 0.01f * ModBus::readInt16BE(_modbus, 5);
        // _data.loadPower = ModBus::readInt16BE(_modbus, 6);

        _data.panelVoltage = 0.1f * ModBus::readInt16BE(_modbus, 7);
        _data.panelCurrent = 0.01f * ModBus::readInt16BE(_modbus, 8);
        // _data.panelPower = ModBus::readInt16BE(_modbus, 9);

        _data.generation = ModBus::readInt16BE(_modbus, 19);
        _data.consumption = ModBus::readInt16BE(_modbus, 20);

        _data.total = ModBus::readInt32BE(_modbus, 28);

        _data.loadEnabled = ModBus::readInt8Upper(_modbus, 32) & 0x80;
        _data.chargingState = ModBus::readInt8Lower(_modbus, 32);

        _data.errorState = ModBus::readInt32BE(_modbus, 33);

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
            RS_DEBUGF("[Renogy] Could not turn load %s: %s (0x%02X)\n", enable ? "on" : "off",
                ModBus::resultToString(result), result);
        }
    }

    void updateUI(JsonDocument& json) const override
    {
        auto battery = json["b"];
        battery["ch"] = _data.batteryCharge;
        battery["vo"] = _data.batteryVoltage;
        battery["cu"] = _data.batteryCurrent;
        battery["te"] = _data.batteryTemperature;
        battery["ge"] = _data.generation;
        battery["co"] = _data.consumption;
        battery["to"] = _data.total;

        auto load = json["l"];
        load["vo"] = _data.loadVoltage;
        load["cu"] = _data.loadCurrent;

        auto panel = json["p"];
        panel["vo"] = _data.panelVoltage;
        panel["cu"] = _data.panelCurrent;

        auto controller = json["c"];
        controller["st"] = _data.chargingState;
        controller["er"] = _data.errorState;
        controller["te"] = _data.controllerTemperature;

        auto output = json["o"];
        output["l"] = _data.loadEnabled;
    }

    void publishHaDiscovery(MqttInterface& mqtt) const override
    {
        // Battery related
        mqtt.publishSensorDiscovery("Battery SOC", "batsoc", "battery", "%", "measurement", "{{value_json.b.ch}}");
        mqtt.publishSensorDiscovery(
            "Battery Voltage", "batvol", "voltage", "V", "measurement", "{{value_json.b.vo|round(1)}}", "mdi:battery");
        mqtt.publishSensorDiscovery(
            "Battery Current", "batcur", "current", "A", "measurement", "{{value_json.b.cu|round(1)}}", "mdi:battery");
        mqtt.publishSensorDiscovery(
            "Battery Temperature", "battem", "temperature", "°C", "measurement", "{{value_json.b.te}}", "mdi:battery");

        mqtt.publishSensorDiscovery(
            "Generation", "engen", "energy", "Wh", "total_increasing", "{{value_json.b.ge}}", "mdi:plus");
        mqtt.publishSensorDiscovery(
            "Consumption", "encon", "energy", "Wh", "total_increasing", "{{value_json.b.co}}", "mdi:minus");

        // Load related
        mqtt.publishSensorDiscovery("Load Voltage", "loavol", "voltage", "V", "measurement",
            "{{value_json.l.vo|round(1)}}", "mdi:alpha-l-box-outline");
        mqtt.publishSensorDiscovery("Load Current", "loacur", "current", "A", "measurement",
            "{{value_json.l.cu|round(1)}}", "mdi:alpha-l-box-outline");
        mqtt.publishSensorDiscovery("Load Power", "loapow", "power", "W", "measurement",
            "{{(value_json.l.vo*value_json.l.cu)|round(1)}}", "mdi:alpha-l-box-outline");

        // Panel related
        mqtt.publishSensorDiscovery("Panel Voltage", "panvol", "voltage", "V", "measurement",
            "{{value_json.p.vo|round(1)}}", "mdi:solar-panel");
        mqtt.publishSensorDiscovery("Panel Current", "pancur", "current", "A", "measurement",
            "{{value_json.p.cu|round(1)}}", "mdi:solar-panel");
        mqtt.publishSensorDiscovery("Panel Power", "panpow", "power", "W", "measurement",
            "{{(value_json.p.vo*value_json.p.cu)|round(1)}}", "mdi:solar-panel");

        // Controller related
        mqtt.publishSensorDiscovery("Controller State", "consta",
            "{{['Unknown',"
            "'Deactivated',"
            "'Activated',"
            "'MPPT',"
            "'Equalizing',"
            "'Boost',"
            "'Floating',"
            "'Overpower'][value_json.c.st|int(-1)+1]}}",
            "mdi:server");
        mqtt.publishSensorDiscovery("Controller Error", "conerr", "{{value_json.c.er}}", "mdi:server");
        mqtt.publishSensorDiscovery("Controller Temperature", "contem", "temperature", "°C", "measurement",
            "{{value_json.c.te}}", "mdi:server");

        // Telemetry
        mqtt.publishSensorDiscovery("RSSI", "rssi", "signal_strength", "dBm", "measurement", "{{value_json.rssi}}");

        // Output (incl. load)
        // TODO command_topic
        mqtt.publishSwitchDiscovery(
            "Load", "ol", "{{'true' if value_json.o.l else 'false'}}", "mdi:alpha-l-box-outline");
        mqtt.publishSwitchDiscovery(
            "Out 1", "o1", "{{'true' if value_json.o.o1 else 'false'}}", "mdi:numeric-1-box-outline");
        mqtt.publishSwitchDiscovery(
            "Out 2", "o2", "{{'true' if value_json.o.o2 else 'false'}}", "mdi:numeric-2-box-outline");
        mqtt.publishSwitchDiscovery(
            "Out 3", "o3", "{{'true' if value_json.o.o3 else 'false'}}", "mdi:numeric-3-box-outline");
    }

    void publishIndividualMqttData(MqttInterface& mqtt) const override
    {
        mqtt.publishSub("/battery/charge", String(_data.batteryCharge).c_str(), false);
        mqtt.publishSub("/battery/voltage", String(_data.batteryVoltage).c_str(), false);
        mqtt.publishSub("/battery/current", String(_data.batteryCurrent).c_str(), false);
        mqtt.publishSub("/battery/temperature", String(_data.batteryTemperature).c_str(), false);
        mqtt.publishSub("/battery/consumption", String(_data.consumption).c_str(), false);
        mqtt.publishSub("/battery/generation", String(_data.generation).c_str(), false);

        mqtt.publishSub("/load/voltage", String(_data.loadVoltage).c_str(), false);
        mqtt.publishSub("/load/current", String(_data.loadCurrent).c_str(), false);

        mqtt.publishSub("/panel/voltage", String(_data.panelVoltage).c_str(), false);
        mqtt.publishSub("/panel/current", String(_data.panelCurrent).c_str(), false);

        mqtt.publishSub("/controller/state", String(_data.chargingState).c_str(), false);
        mqtt.publishSub("/controller/error", String(_data.errorState).c_str(), false);
        mqtt.publishSub("/controller/temperature", String(_data.controllerTemperature).c_str(), false);
    }

    float getValueForType(const InputType type) const override
    {
        switch (type)
        {
        case InputType::bsoc:
            return _data.batteryCharge;
        case InputType::bvoltage:
            return _data.batteryVoltage;
        case InputType::btemp:
            return _data.batteryTemperature;
        case InputType::pvoltage:
            return _data.panelVoltage;
        case InputType::pcurrent:
            return _data.panelCurrent;
        case InputType::ctemp:
            return _data.controllerTemperature;

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
            RS_DEBUGF("[Renogy] Model: %s, SWV: %d, HWV: %d, S#: %d addr: %d, ProtV: %d\n", model.c_str(),
                ModBus::readInt32BE(_modbus, 8), ModBus::readInt32BE(_modbus, 10), ModBus::readInt32BE(_modbus, 12),
                ModBus::readInt8Lower(_modbus, 14), ModBus::readInt32BE(_modbus, 15));
        }
        else
        {
            RS_DEBUGF("[Renogy] Could not read registers: %d\n", result);
        }
    }

private:
    ModbusMaster _modbus;
    String model = "";
}; // class Renogy

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