#include "Renogy.h"

#include "Constants.h"
#include "MQTT.h"
#include "PVOutput.h"

namespace ModBus
{
    int8_t readInt8Lower(ModbusMaster& modbus, const uint8_t startAddress)
    {
        return (modbus.getResponseBuffer(startAddress) & 0xFF);
    }

    int8_t readInt8Upper(ModbusMaster& modbus, const uint8_t startAddress)
    {
        return ((modbus.getResponseBuffer(startAddress) >> 8) & 0xFF);
    }

    int16_t readInt16BE(ModbusMaster& modbus, const uint8_t startAddress)
    {
        return modbus.getResponseBuffer(startAddress);
    }

    int16_t readInt16LE(ModbusMaster& modbus, const uint8_t startAddress)
    {
        const uint16_t reg = readInt16BE(modbus, startAddress);
        return ((reg << 8) & 0xFF00) | ((reg >> 8) & 0x00FF);
    }

    int32_t readInt32BE(ModbusMaster& modbus, const uint8_t startAddress)
    {
        return (modbus.getResponseBuffer(2 + startAddress) & 0xFFFF)
            | ((modbus.getResponseBuffer(startAddress) & 0xFFFF) << 16);
    }

    int32_t readInt32LE(ModbusMaster& modbus, const uint8_t startAddress)
    {
        uint32_t reg = readInt32BE(modbus, startAddress);
        return ((reg << 8) & 0xFF00FF00) | ((reg >> 8) & 0x00FF00FF);
    }

    String readString(ModbusMaster& modbus, const uint8_t startAddress, const uint8_t registers)
    {
        String str = "";
        for (uint8_t i = 0; i < registers; ++i)
        {
            str += static_cast<char>(readInt8Upper(modbus, i));
            str += static_cast<char>(readInt8Lower(modbus, i));
        }
        return str;
    }

} // namespace ModBus

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
// 0x0113 (2) 19 - power generation of the current day actual value
// 0x0114 (2) 20 - power consumption of the current day actual value
//
// Historical Information
//
// 0x0115 (2) 21 - total number of operating days
// 0x0116 (2) 22 - total number of battery over-discharges
// 0x0117 (2) 23 - total number of battery full discharges
// 0x0118 (4) 24 - total charging amp-hrs of the battery actual value
// 0x011A (4) 26 - total discharging amp-hrs of the battery actual value
// 0x011C (4) 28 - cumulative power generation actual value
// 0x011E (4) 30 - cumulative power consumption actual value
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

#ifdef DEMO_MODE
uint8_t batteryCharge = 0;
bool batteryDirection = true;
#endif

float batterySocToVolts(const float soc)
{
    return 0.000004 * soc * soc * soc - 0.000848 * soc * soc + 0.061113 * soc + 10.6099;
}

void Renogy::readAndProcessData()
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

    // update listener
    if (_listener)
    {
        _listener(_data);
    }

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

    // update listener
    if (_listener)
    {
        _listener(_data);
    }

#else
    // Read 34 registers starting at 0x0100)
    _modbus.clearResponseBuffer();
    const uint8_t result = _modbus.readHoldingRegisters(0x0100, 34);

    if (result == _modbus.ku8MBSuccess)
    {
        _data.batteryCharge = ModBus::readInt16BE(_modbus, 0);
        _data.batteryVoltage = 0.1f * ModBus::readInt16BE(_modbus, 1);
        _data.batteryCurrent = 0.01f * ModBus::readInt16BE(_modbus, 2);
        _data.controllerTemperature = ModBus::readInt8Upper(_modbus, 3);
        _data.batteryTemperature = ModBus::readInt8Lower(_modbus, 3);

        _data.loadVoltage = 0.1f * ModBus::readInt16BE(_modbus, 4);
        _data.loadCurrent = 0.01f * ModBus::readInt16BE(_modbus, 5);
        // _data.loadPower = ModBus::readInt16BE(_modbus, 6);

        _data.panelVoltage = 0.1f * ModBus::readInt16BE(_modbus, 7);
        _data.panelCurrent = 0.01f * ModBus::readInt16BE(_modbus, 8);
        // _data.panelPower = ModBus::readInt16BE(_modbus, 9);

        _data.generation = ModBus::readInt16BE(_modbus, 19);
        _data.consumption = ModBus::readInt16BE(_modbus, 20);

        _data.loadEnabled = ModBus::readInt8Upper(_modbus, 32) & 0x80;
        _data.chargingState = ModBus::readInt8Lower(_modbus, 32);

        _data.errorState = ModBus::readInt32BE(_modbus, 33);

        // update listener
        if (_listener)
        {
            _listener(_data);
        }

        if (model.isEmpty())
        {
            readModel();
        }
    }
    else
    {
        RNG_DEBUGF("[Renogy] Could not read registers: %d\n", result);
    }
#endif
}

void Renogy::enableLoad(const bool enable)
{
#ifdef DEMO_MODE
    _data.loadEnabled = enable;
#else
    const uint8_t result = _modbus.writeSingleRegister(0x010A, enable ? 0x01 : 0x00);
    if (result != _modbus.ku8MBSuccess)
    {
        RNG_DEBUGF("[Renogy] Could not turn load %s: %d\n", enable ? "on" : "off", result);
    }
#endif
}

void Renogy::setListener(DataListener listener)
{
    _listener = listener;
}

void Renogy::readModel()
{
    _modbus.clearResponseBuffer();
    const uint8_t result = _modbus.readHoldingRegisters(0x000C, 19);

    if (result == _modbus.ku8MBSuccess)
    {
        model = ModBus::readString(_modbus, 0, 8);
        RNG_DEBUGF("[Renogy] Model: %s, SWV: %d, HWV: %d, S#: %d addr: %d, ProtV: %d\n", model.c_str(),
            ModBus::readInt32BE(_modbus, 8), ModBus::readInt32BE(_modbus, 10), ModBus::readInt32BE(_modbus, 12),
            ModBus::readInt8Lower(_modbus, 14), ModBus::readInt32BE(_modbus, 15));
    }
    else
    {
        RNG_DEBUGF("[Renogy] Could not read registers: %d\n", result);
    }
}