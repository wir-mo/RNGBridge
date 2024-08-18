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

    uint16_t readUInt16BE(ModbusMaster& modbus, const uint8_t startAddress)
    {
        return modbus.getResponseBuffer(startAddress);
    }

    int16_t readInt16BE(ModbusMaster& modbus, const uint8_t startAddress)
    {
        return modbus.getResponseBuffer(startAddress);
    }

    uint16_t readUInt16LE(ModbusMaster& modbus, const uint8_t startAddress)
    {
        const uint16_t reg = readInt16BE(modbus, startAddress);
        return ((reg << 8) & 0xFF00) | ((reg >> 8) & 0x00FF);
    }

    int16_t readInt16LE(ModbusMaster& modbus, const uint8_t startAddress)
    {
        return readUInt16LE(modbus, startAddress);
    }

    uint32_t readUInt32BE(ModbusMaster& modbus, const uint8_t startAddress)
    {
        return ((modbus.getResponseBuffer(startAddress) & 0xFFFF) << 16)
            | (modbus.getResponseBuffer(1 + startAddress) & 0xFFFF);
    }

    int32_t readInt32BE(ModbusMaster& modbus, const uint8_t startAddress)
    {
        return readUInt32BE(modbus, startAddress);
    }

    uint32_t readUInt32LE(ModbusMaster& modbus, const uint8_t startAddress)
    {
        const uint32_t reg = readInt32BE(modbus, startAddress);
        return ((reg << 8) & 0xFF00FF00) | ((reg >> 8) & 0x00FF00FF);
    }

    int32_t readInt32LE(ModbusMaster& modbus, const uint8_t startAddress)
    {
        return readUInt32LE(modbus, startAddress);
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

// 0x3100 (2) B1  - PV array input voltage * 0.01 (V)
// 0x3101 (2) B2  - PV array input current * 0.01 (A)
// 0x3102 (2) B3  - PV array input power L * 0.01 (W)
// 0x3103 (2) B4  - PV array input power H * 0.01 (W)
//
// 0x3104 (2) B5  - Battery input voltage * 0.01 (V)
// 0x3105 (2) B6  - Battery input current * 0.01 (A)
// 0x3106 (2) B7  - Battery charging power L * 0.01 (W)
// 0x3107 (2) B8  - Battery charging power H * 0.01 (W)
//
// 0x310C (2) B13 - Load voltage * 0.01 (V)
// 0x310D (2) B14 - Load current * 0.01 (A)
// 0x310E (2) B15 - Load power L * 0.01 (W)
// 0x310F (2) B16 - Load power H * 0.01 (W)
//
// 0x3110 (2) B17 - Battery temperature * 0.01 (°C)
// 0x3111 (2) B18 - Controller temperature * 0.01 (°C)
//
// 0x311A (2) B27 - Battery SOC * 0.01 (%)
// 0x311B (2) B28 - Remote battery temperature * 0.01 (°C)
//
// 0x311D (2) B30 - System voltage * 0.01 (V)
//
// 0x3200 (2) C1  - Battery status
//            - lower 4 bits (0-3)
//            00H: normal
//            01H: overvolt
//            02H: undervolt
//            03H: low volt disconnect
//            04H: fault
//
//            - lower 4 bits (4-7)
//            00H: normal
//            01H: overtemp (higher than warning setting)
//            02H: low temp (lower than warning setting)
//
//            - bit 8
//            00H: normal
//            01H: abnormal
//
//            - bit 15
//            00H: normal
//            01H: wrong identification for rated voltage
//
// 0x3201 (2) C2  - Charger status
//            - D00
//            00H: standby
//            01H: running
//
//            - D01
//            00H: normal
//            01H: fault
//
//            - D02 - D03
//            00H: no charging
//            01H: float
//            02H: boost
//            03H: equalization
//
//            - D04: pv input short
//
//            - D07: load mosfet short
//            - D08: load short
//            - D09: load over current
//            - D10: input over current
//            - D11: anti reverse mosfet short
//            - D12: charging or anti reverse mosfet short
//            - D13: charging mosfet short
//
//            - D14 - D15: input voltage status
//            00H: normal
//            01H: no power connected
//            02H: higher volt input
//            03H: input volt error
//
// 0x3201 (2) C7  - Discharger status
//            - D00
//            00H: standby
//            01H: running
//
//            - D01
//            00H: normal
//            01H: fault
//
//            - D04: output overpressure
//            - D05: boost overpressure
//            - D06: high voltage side short circuit
//            - D07: input overpressure
//            - D08: output voltage abnormal
//            - D09: unable to stop discharging
//            - D10: unable to discharge
//            - D11: short circuit
//
//            - D12 - D13: output power
//            00H: light load
//            01H: moderate
//            02H: rated
//            03H: overload
//
//            - D14 - D15: input voltage status
//            00H: normal
//            01H: low
//            02H: high
//            03H: no access, input voltage error
//

#ifdef DEMO_MODE
uint8_t batteryCharge = 0;
bool batteryDirection = true;
#endif

constexpr float batterySocToVolts(const float soc)
{
    return 0.000004 * soc * soc * soc - 0.000848 * soc * soc + 0.061113 * soc + 10.6099;
}

constexpr const char* mbResultToString(const uint8_t result)
{
    if (result == ModbusMaster::ku8MBIllegalFunction)
    {
        return "IllegalFunction";
    }
    if (result == ModbusMaster::ku8MBIllegalDataAddress)
    {
        return "IllegalDataAddress";
    }
    if (result == ModbusMaster::ku8MBIllegalDataValue)
    {
        return "IllegalDataValue";
    }
    if (result == ModbusMaster::ku8MBSlaveDeviceFailure)
    {
        return "SalveDeviceFailure";
    }
    if (result == ModbusMaster::ku8MBInvalidSlaveID)
    {
        return "InvalidSlaveID";
    }
    if (result == ModbusMaster::ku8MBInvalidFunction)
    {
        return "InvalidFunction";
    }
    if (result == ModbusMaster::ku8MBResponseTimedOut)
    {
        return "ResponseTimedOut";
    }
    if (result == ModbusMaster::ku8MBInvalidCRC)
    {
        return "InvalidCRC";
    }
    return "Unknown";
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
    // Read 18 registers starting at 0x3100
    _modbus.clearResponseBuffer();
    uint8_t result = _modbus.readInputRegisters(0x3100, 18);
    if (result != _modbus.ku8MBSuccess)
    {
        RNG_DEBUGF("[Renogy] Could not read registers 0x3100: %s (0x%02X)\n", mbResultToString(result), result);
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
        RNG_DEBUGF("[Renogy] Could not read registers 0x311A: %s (0x%02X)\n", mbResultToString(result), result);
        return;
    }
    _data.batteryCharge = 0.01f * ModBus::readInt16BE(_modbus, 0);

    // Read 3 registers starting at 0x3200
    _modbus.clearResponseBuffer();
    result = _modbus.readInputRegisters(0x3200, 3);
    if (result != _modbus.ku8MBSuccess)
    {
        RNG_DEBUGF("[Renogy] Could not read registers 0x3200: %s (0x%02X)\n", mbResultToString(result), result);
        return;
    }
    _data.chargingState = 0x3 & (ModBus::readInt8Lower(_modbus, 1) >> 2);
    // _data.errorState = ModBus::readInt32BE(_modbus, 33);

    // Read 10 registers starting at 0x330A
    _modbus.clearResponseBuffer();
    result = _modbus.readInputRegisters(0x330A, 10);
    if (result != _modbus.ku8MBSuccess)
    {
        RNG_DEBUGF("[Renogy] Could not read registers 0x330A: %s (0x%02X)\n", mbResultToString(result), result);
        return;
    }

    _data.consumption = ModBus::readInt32BE(_modbus, 0);
    _data.generation = ModBus::readInt32BE(_modbus, 8);

    // Read 1 register at 0x02
    _modbus.clearResponseBuffer();
    result = _modbus.readCoils(0x02, 1);
    if (result != _modbus.ku8MBSuccess)
    {
        RNG_DEBUGF("[Renogy] Could not read registers 0x02: %s (0x%02X)\n", mbResultToString(result), result);
        return;
    }
    _data.loadEnabled = ModBus::readUInt16BE(_modbus, 0);

    // update listener
    if (_listener)
    {
        _listener(_data);
    }

    // if (model.isEmpty())
    // {
    //     readModel();
    // }
#endif
}

void Renogy::enableLoad(const bool enable)
{
#ifdef DEMO_MODE
    _data.loadEnabled = enable;
#else
    const uint8_t result = _modbus.writeSingleCoil(0x02, enable ? 0x01 : 0x00);
    if (result != _modbus.ku8MBSuccess)
    {
        RNG_DEBUGF(
            "[Renogy] Could not turn load %s: %s (0x%02X)\n", enable ? "on" : "off", mbResultToString(result), result);
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