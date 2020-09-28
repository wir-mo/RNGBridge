#pragma once

#include <ESPUI.h>
#include <Ticker.h>


namespace GUI
{
    namespace Callback
    {
        extern void updateMQTTIP(Control* sender, int type);

        extern void updateMQTTPort(Control* sender, int type);

        extern void updateMQTTTopic(Control* sender, int type);

        extern void updateMQTTEnable(Control* sender, int type);

        extern void updateWifiSSID(Control* sender, int type);

        extern void updateWifiPassword(Control* sender, int type);

        extern void updateWifiEnable(Control* sender, int type);

        extern void updateSystemID(Control* sender, int type);

        extern void updateAPIKey(Control* sender, int type);

        extern void updateTimeOffset(Control* sender, int type);

        extern void updatePVOutputEnable(Control* sender, int type);
    } // namespace Callback

    extern void setup();

    extern void update(const uint8_t charge, const float batteryVoltage, const float batteryCurrent,
        const int8_t batteryTemperature, const int8_t controllerTemperature, const float laodVoltage,
        const float laodCurrent, const int16_t loadPower, const float panelVoltage, const float panelCurrent,
        const int16_t panelPower, const int8_t chargingState, const int32_t errorState);

    extern void updateWiFiStatus(const String& status);

    extern void updateMQTTStatus(const String& status);

    extern void updatePVOutputStatus(const String& status);

    extern size_t clients();

    extern int chargeLabel;
    extern int BVLabel;
    extern int BCLabel;
    extern int BTLabel;
    extern int CTLabel;
    extern int LVLabel;
    extern int LCLabel;
    extern int LWLabel;
    extern int PVLabel;
    extern int PCLabel;
    extern int PWLabel;
    extern int CSLabel;
    extern int ELabel;
    extern int mqttStatusLabel;
    extern int wifiStatusLabel;
    extern int pvOutputStatusLabel;

    extern Ticker delayedUpdate;
} // namespace GUI