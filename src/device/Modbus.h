#pragma once
#include <cstdint>

#include <ModbusMaster.h>

namespace ModBus
{
    inline int8_t readInt8Lower(ModbusMaster& modbus, const uint8_t startAddress)
    {
        return (modbus.getResponseBuffer(startAddress) & 0xFF);
    }

    inline int8_t readInt8Upper(ModbusMaster& modbus, const uint8_t startAddress)
    {
        return ((modbus.getResponseBuffer(startAddress) >> 8) & 0xFF);
    }

    inline uint16_t readUInt16BE(ModbusMaster& modbus, const uint8_t startAddress)
    {
        return modbus.getResponseBuffer(startAddress);
    }

    inline int16_t readInt16BE(ModbusMaster& modbus, const uint8_t startAddress)
    {
        return modbus.getResponseBuffer(startAddress);
    }

    inline uint16_t readUInt16LE(ModbusMaster& modbus, const uint8_t startAddress)
    {
        const uint16_t reg = readInt16BE(modbus, startAddress);
        return ((reg << 8) & 0xFF00) | ((reg >> 8) & 0x00FF);
    }

    inline int16_t readInt16LE(ModbusMaster& modbus, const uint8_t startAddress)
    {
        return readUInt16LE(modbus, startAddress);
    }

    inline uint32_t readUInt32BE(ModbusMaster& modbus, const uint8_t startAddress)
    {
        return ((modbus.getResponseBuffer(startAddress) & 0xFFFF) << 16)
            | (modbus.getResponseBuffer(1 + startAddress) & 0xFFFF);
    }

    inline int32_t readInt32BE(ModbusMaster& modbus, const uint8_t startAddress)
    {
        return readUInt32BE(modbus, startAddress);
    }

    inline uint32_t readUInt32LE(ModbusMaster& modbus, const uint8_t startAddress)
    {
        const uint32_t reg = readInt32BE(modbus, startAddress);
        return ((reg << 8) & 0xFF00FF00) | ((reg >> 8) & 0x00FF00FF);
    }

    inline int32_t readInt32LE(ModbusMaster& modbus, const uint8_t startAddress)
    {
        return readUInt32LE(modbus, startAddress);
    }

    inline String readString(ModbusMaster& modbus, const uint8_t startAddress, const uint8_t registers)
    {
        String str = "";
        for (uint8_t i = 0; i < registers; ++i)
        {
            str += static_cast<char>(readInt8Upper(modbus, i));
            str += static_cast<char>(readInt8Lower(modbus, i));
        }
        return str;
    }

    constexpr const char* resultToString(const uint8_t result)
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
} // namespace ModBus
