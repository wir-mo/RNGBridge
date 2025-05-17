#pragma once

#include <functional>

#include <HardwareSerial.h>
#include <ModbusMaster.h>

#include "Constants.h"
#include "MQTT.h"
#include "Modbus.h"
#include "PVOutput.h"
#include "RSDevice.h"

class Dummy : public RSDevice
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
    /// @brief Construct a new Dummy object
    Dummy() { }

    virtual ~Dummy() override = default;

    Dummy(Dummy&&) = delete;

    constexpr float batterySocToVolts(const float soc)
    {
        return 0.000004 * soc * soc * soc - 0.000848 * soc * soc + 0.061113 * soc + 10.6099;
    }

    void readAndProcessData() override
    {
#if DEMO_MODE == SIMULATED_DEMO_DATA

        // constant 100W supply
        // P = U * I | I = P / U
        _data.panelVoltage = 14.0f + 0.1f * random(-10, 10);
        _data.panelCurrent = 100.0f / _data.panelVoltage;
        // _data.panelPower = floor(_data.panelVoltage * _data.panelCurrent);

        batteryDirection ? ++batteryCharge : --batteryCharge;
        if (batteryCharge == 0 || batteryCharge == 100)
        {
            batteryDirection = !batteryDirection;
        }
        _data.batteryCharge = batteryCharge;
        _data.batteryVoltage = batterySocToVolts(_data.batteryCharge); // 0.1f * random(100, 140);
        _data.batteryCurrent = _data.loadEnabled ? (batteryDirection ? (3.0f + 0.01f * random(-100, 100)) : 0.0f)
                                                 : (100.0f / _data.batteryVoltage);
        _data.controllerTemperature = 21 + random(-2, 2);
        _data.batteryTemperature = 20 + random(-2, 2);

        if (_data.loadEnabled)
        {
            // P = U * I
            _data.loadVoltage = _data.batteryVoltage;
            _data.loadCurrent = (100.0f / _data.batteryVoltage) - _data.batteryCurrent;
            // _data.loadPower = floor(_data.loadVoltage * _data.loadCurrent);
        }
        else
        {
            _data.loadVoltage = _data.batteryVoltage;
            _data.loadCurrent = 0.0f;
            // _data.loadPower = 0;
        }

        _data.chargingState = batteryDirection ? 0x01 : 0x00;

        _data.errorState = _data.batteryVoltage <= 11 ? 0x10000 : 0x0;

#elif DEMO_MODE == CONST_DEMO_DATA
        constexpr static const float PANEL_POWER = 100.0f;
        _data.panelVoltage = 14.25f;
        _data.panelCurrent = PANEL_POWER / _data.panelVoltage;
        // _data.panelPower = floor(_data.panelVoltage * _data.panelCurrent);

        _data.batteryCharge = 80.0f;
        _data.batteryVoltage = batterySocToVolts(_data.batteryCharge);
        _data.batteryCurrent = _data.loadEnabled ? 3.45f : PANEL_POWER / _data.batteryVoltage;

        _data.controllerTemperature = 21;
        _data.batteryTemperature = 20;

        if (_data.loadEnabled)
        {
            // P = U * I
            _data.loadVoltage = _data.batteryVoltage;
            _data.loadCurrent = (PANEL_POWER / _data.batteryVoltage) - _data.batteryCurrent;
            // _data.loadPower = floor(_data.loadVoltage * _data.loadCurrent);
        }
        else
        {
            _data.loadVoltage = _data.batteryVoltage;
            _data.loadCurrent = 0.0f;
            // _data.loadPower = 0;
        }

        _data.chargingState = 2; // 2 = mppt

        // _data.errorState = _data.batteryVoltage <= 11 ? 0x10000 : 0x0;
        _data.errorState = 0x400000 | 0x20000; //  Ambient temperature too high | Battery over-voltage
#endif

        // All data was read so notify the listener
        notifyListener();
    }

    void enableLoad(const bool enable) override { _data.loadEnabled = enable; }

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
    String model = "Dummy";
    uint8_t batteryCharge = 0;
    bool batteryDirection = true;
}; // class Dummy
