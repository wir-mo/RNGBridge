#pragma once

#include <ModbusMaster.h>
#include <Ticker.h>

extern void blinkLED();

namespace Renogy
{
    namespace Callback
    {

        extern void readAndProcessData();

    } // namespace Callback

    namespace ModBus
    {
        extern int8_t readInt8Lower(ModbusMaster& modbus, const uint8_t startAddress);

        extern int8_t readInt8Upper(ModbusMaster& modbus, const uint8_t startAddress);

        extern int16_t readInt16BE(ModbusMaster& modbus, const uint8_t startAddress);

        extern int16_t readInt16LE(ModbusMaster& modbus, const uint8_t startAddress);

        extern int32_t readInt32BE(ModbusMaster& modbus, const uint8_t startAddress);

        extern int32_t readInt32LE(ModbusMaster& modbus, const uint8_t startAddress);

        extern ModbusMaster modbus;
    } // namespace ModBus

    extern void setup();

    extern void writeException(const uint8_t code);

    extern Ticker readDataTimer;

} // namespace Renogy
