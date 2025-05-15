#pragma once

#include <functional>

#include <HardwareSerial.h>
#include <ModbusMaster.h>

#include "Modbus.h"
#include "RSDevice.h"

class Epever : public RSDevice
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
    /// @brief Construct a new Epever object
    /// @param serial Hardware Serial for ModBus communication
    /// @param address Modbus device address
    Epever(HardwareSerial& serial, const uint8_t address)
    {
        serial.setTimeout(100);
        // Modbus at 115200 baud
        serial.begin(115200);
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

    virtual ~Epever() override = default;

    Epever(Epever&&) = delete;

    void readAndProcessData() override
    {
        // Read 18 registers starting at 0x3100
        _modbus.clearResponseBuffer();
        uint8_t result = _modbus.readInputRegisters(0x3100, 18);
        if (result != _modbus.ku8MBSuccess)
        {
            RNG_DEBUGF(
                "[Epever] Could not read registers 0x3100: %s (0x%02X)\n", ModBus::resultToString(result), result);
            return;
        }

        _data.panelVoltage = 0.01f * ModBus::readInt16BE(_modbus, 0);
        _data.panelCurrent = 0.01f * ModBus::readInt16BE(_modbus, 1);
        // _data.panelPower = 0.01f * ModBus::readInt32BE(_modbus, 2);

        _data.batteryVoltage = 0.01f * ModBus::readInt16BE(_modbus, 4);
        _data.batteryCurrent = 0.01f * ModBus::readInt16BE(_modbus, 5);
        // _data.batteryPower = 0.01f * ModBus::readInt32BE(_modbus, 6);

        _data.loadVoltage = 0.01f * ModBus::readInt16BE(_modbus, 12);
        _data.loadCurrent = 0.01f * ModBus::readInt16BE(_modbus, 13);
        // _data.loadPower = 0.01f * ModBus::readInt32BE(_modbus, 14);

        _data.batteryTemperature = 0.01f * ModBus::readInt16BE(_modbus, 16);
        _data.controllerTemperature = 0.01f * ModBus::readInt16BE(_modbus, 17);

        // Read 1 register at 0x311A
        _modbus.clearResponseBuffer();
        result = _modbus.readInputRegisters(0x311A, 1);
        if (result != _modbus.ku8MBSuccess)
        {
            RNG_DEBUGF(
                "[Epever] Could not read registers 0x311A: %s (0x%02X)\n", ModBus::resultToString(result), result);
            return;
        }
        _data.batteryCharge = 0.01f * ModBus::readInt16BE(_modbus, 0);

        // Read 3 registers starting at 0x3200
        _modbus.clearResponseBuffer();
        result = _modbus.readInputRegisters(0x3200, 3);
        if (result != _modbus.ku8MBSuccess)
        {
            RNG_DEBUGF(
                "[Epever] Could not read registers 0x3200: %s (0x%02X)\n", ModBus::resultToString(result), result);
            return;
        }
        _data.chargingState = 0x3 & (ModBus::readInt8Lower(_modbus, 1) >> 2);
        // _data.errorState = ModBus::readInt32BE(_modbus, 33);

        // Read 10 registers starting at 0x330A
        _modbus.clearResponseBuffer();
        result = _modbus.readInputRegisters(0x330A, 10);
        if (result != _modbus.ku8MBSuccess)
        {
            RNG_DEBUGF(
                "[Epever] Could not read registers 0x330A: %s (0x%02X)\n", ModBus::resultToString(result), result);
            return;
        }

        _data.consumption = ModBus::readInt32BE(_modbus, 0);
        _data.generation = ModBus::readInt32BE(_modbus, 8);

        // Read 1 register at 0x02
        _modbus.clearResponseBuffer();
        result = _modbus.readCoils(0x02, 1);
        if (result != _modbus.ku8MBSuccess)
        {
            RNG_DEBUGF("[Epever] Could not read registers 0x02: %s (0x%02X)\n", ModBus::resultToString(result), result);
            return;
        }
        _data.loadEnabled = ModBus::readUInt16BE(_modbus, 0);

        // All data was read so notify the listener
        notifyListener();

        // if (model.isEmpty())
        // {
        //     readModel();
        // }
    }

    void readModel()
    {
        _modbus.clearResponseBuffer();
        const uint8_t result = _modbus.readHoldingRegisters(0x000C, 19);

        if (result == _modbus.ku8MBSuccess)
        {
            model = ModBus::readString(_modbus, 0, 8);
            RNG_DEBUGF("[Epever] Model: %s, SWV: %d, HWV: %d, S#: %d addr: %d, ProtV: %d\n", model.c_str(),
                ModBus::readInt32BE(_modbus, 8), ModBus::readInt32BE(_modbus, 10), ModBus::readInt32BE(_modbus, 12),
                ModBus::readInt8Lower(_modbus, 14), ModBus::readInt32BE(_modbus, 15));
        }
        else
        {
            RNG_DEBUGF("[Epever] Could not read registers: %d\n", result);
        }
    }

    void enableLoad(const bool enable) override
    {
#ifdef DEMO_MODE
        _data.loadEnabled = enable;
#else
        const uint8_t result = _modbus.writeSingleCoil(0x02, enable ? 0x01 : 0x00);
        if (result != _modbus.ku8MBSuccess)
        {
            RNG_DEBUGF("[Epever] Could not turn load %s: %s (0x%02X)\n", enable ? "on" : "off",
                ModBus::resultToString(result), result);
        }
#endif
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
    ModbusMaster _modbus;
    String model = "";
}; // class Epever
