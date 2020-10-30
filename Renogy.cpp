#include "Constants.h"
#include "GUI.h"
#include "MQTT.h"
#include "PVOutput.h"
#include "Renogy.h"
#include "Settings.h"
#include "WIFI.h"


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
//            - upper 8 bits are street light status and brightness.
//
// 0x0121 (4) 33 - controller fault and warning information
//            - 32 bit value of flags
//
//                B31: Reserved
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

void blinkLED()
{
    digitalWrite(LED_BUILTIN, LOW);
    delay(100);
    digitalWrite(LED_BUILTIN, HIGH);
}

namespace Renogy
{
    namespace Callback
    {
        void readAndProcessData()
        {
            // Read 30 registers starting at 0x0100)
            ModBus::modbus.clearResponseBuffer();
            const uint8_t result = ModBus::modbus.readHoldingRegisters(0x0100, 34);

            if (result == ModBus::modbus.ku8MBSuccess)
            {
                uint8_t charge = ModBus::readInt16BE(ModBus::modbus, 0);
                float batteryVoltage = 0.1 * ModBus::readInt16BE(ModBus::modbus, 1);
                float batteryCurrent = 0.01 * ModBus::readInt16BE(ModBus::modbus, 2);
                int8_t controllerTemperature = ModBus::readInt8Upper(ModBus::modbus, 3);
                int8_t batteryTemperature = ModBus::readInt8Lower(ModBus::modbus, 3);

                float loadVoltage = 0.1 * ModBus::readInt16BE(ModBus::modbus, 4);
                float loadCurrent = 0.01 * ModBus::readInt16BE(ModBus::modbus, 5);
                int16_t loadPower = ModBus::readInt16BE(ModBus::modbus, 6);

                float panelVoltage = 0.1 * ModBus::readInt16BE(ModBus::modbus, 7);
                float panelCurrent = 0.01 * ModBus::readInt16BE(ModBus::modbus, 8);
                int16_t panelPower = ModBus::readInt16BE(ModBus::modbus, 9);

                int8_t chargingState = ModBus::readInt8Lower(ModBus::modbus, 32);
                // upper are street light status
                int32_t errorState = ModBus::readInt32BE(ModBus::modbus, 33);

                // Send MQTT data if it is enabled
                if (Settings::settings.mqtt)
                {
                    // Max size under assumptions Voltage < 1000, Current < 1000: 261 + null terminator + tolerance
                    //{"device":"0123456789AB","b":{"charge":254,"voltage":999.9,"current":999.99,"temperature":254},"l":{"voltage":999.9,"current":999.99,"power":65534},"p":{"voltage":999.9,"current":999.99,"power":65534},"s":{"state":254,"errorState":4294967295,"temperature":254}}
                    char jsonBuf[280];
                    snprintf_P(jsonBuf, 280, jsonFormat, WIFI::mac.c_str(), charge, batteryVoltage, batteryCurrent,
                        batteryTemperature, loadVoltage, loadCurrent, loadPower, panelVoltage, panelCurrent, panelPower,
                        chargingState, errorState, controllerTemperature);
                    MQTT::publish(jsonBuf);
                }

                // Update PVOutput data if it is enabled
                if (Settings::settings.pvOutput)
                {
                    PVOutput::Callback::updateData(
                        2, panelVoltage * panelCurrent, loadVoltage * loadCurrent, panelVoltage);
                }

                // update ui
                if (GUI::clients() > 0)
                {
                    GUI::update(charge, batteryVoltage, batteryCurrent, batteryTemperature, controllerTemperature,
                        loadVoltage, loadCurrent, loadPower, panelVoltage, panelCurrent, panelPower, chargingState,
                        errorState);
                }
            }
            else
            {
                if (Settings::settings.mqtt)
                {
                    MQTT::publish("{\"device\":\"" + WIFI::mac + "\",\"mbError\":" + String(result) + "}");
                }
            }

            blinkLED();
        }
    } // namespace Callback

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

        ModbusMaster modbus;
    } // namespace ModBus

    void setup()
    {
        // Modbus at 9600 baud
        Serial.begin(9600);
        // Maybe make configurable with updateBaudrate(baud);

        // Renogy Device ID = 1
        ModBus::modbus.begin(1, Serial);
        // Maybe make configurable with begin(x, Serial);

        // Read and process data every 2 seconds
        readDataTimer.attach_scheduled(2, Renogy::Callback::readAndProcessData);
    }

    const char* jsonFormat PROGMEM
        = R"({"device":"%s","b":{"charge":%hhu,"voltage":%.1f,"current":%.2f,"temperature":%hhu},"l":{"voltage":%.1f,"current":%.2f,"power":%hu},"p":{"voltage":%.1f,"current":%.2f,"power":%hu},"s":{"state":%hhu,"errorState":%u,"temperature":%hhu}})";
    Ticker readDataTimer;
} // namespace Renogy