#include "GUI.h"

#include "Constants.h"
#include "MQTT.h"
#include "Settings.h"

namespace GUI
{
    namespace Callback
    {
        void updateMQTTIP(Control* sender, int type)
        {
            IPAddress ip {};
            if (ip.fromString(sender->value))
            {
                Settings::updateMQTTIP(ip);
                MQTT::updateIP(ip);
            }
        }

        void updateMQTTPort(Control* sender, int type)
        {
            const int32_t port = sender->value.toInt();
            // Only accept positive values
            if (port > 0)
            {
                const uint16_t port16 = port & 0xFFFF;
                Settings::updateMQTTPort(port16);
                MQTT::updatePort(port16);
            }
        }

        void updateMQTTTopic(Control* sender, int type)
        {
            const uint8_t len = sender->value.length();
            // Only accept string which are 32 characters max
            if (len > 0 && len <= 32)
            {
                Settings::updateMQTTTopic(sender->value);
                MQTT::updateTopic(sender->value);
            }
        }

        void updateWifiSSID(Control* sender, int type)
        {
            const uint8_t len = sender->value.length();
            if (len > 0 && len <= 32)
            {
                // TODO have specific function in settings namespace
                sender->value.toCharArray(Settings::settings.ssid, 32, 0);
                Settings::store();
            }
        }

        void updateWifiPassword(Control* sender, int type)
        {
            const uint8_t len = sender->value.length();
            if (len > 0 && len <= 32)
            {
                // TODO have specific function in settings namespace
                sender->value.toCharArray(Settings::settings.password, 32, 0);
                Settings::store();
            }
        }

        void connectWifiButton(Control* sender, int type)
        {
            if (type == B_DOWN)
            {
                WIFI::connect(String(Settings::settings.ssid), String(Settings::settings.password));
            }
        }
    } // namespace Callback

    void setup()
    {
        const uint16_t overviewTab = ESPUI.addControl(ControlType::Tab, "Overview", F("Overview"));
        const uint16_t mqttTab = ESPUI.addControl(ControlType::Tab, "MQTT", F("MQTT"));
        const uint16_t wifiTab = ESPUI.addControl(ControlType::Tab, "WiFi", F("WiFi"));

        chargeLabel = ESPUI.addControl(
            ControlType::Label, "Battery capacity", F("unknown"), ControlColor::Emerald, overviewTab);
        BVLabel
            = ESPUI.addControl(ControlType::Label, "Battery voltage", F("unknown"), ControlColor::Emerald, overviewTab);
        BCLabel
            = ESPUI.addControl(ControlType::Label, "Battery current", F("unknown"), ControlColor::Emerald, overviewTab);
        BTLabel = ESPUI.addControl(
            ControlType::Label, "Battery temperature", F("unknown"), ControlColor::Emerald, overviewTab);

        LVLabel = ESPUI.addControl(ControlType::Label, "Load voltage", F("unknown"), ControlColor::Carrot, overviewTab);
        LCLabel = ESPUI.addControl(ControlType::Label, "Load current", F("unknown"), ControlColor::Carrot, overviewTab);
        LWLabel = ESPUI.addControl(ControlType::Label, "Load power", F("unknown"), ControlColor::Carrot, overviewTab);

        PVLabel = ESPUI.addControl(
            ControlType::Label, "Panel voltage", F("unknown"), ControlColor::Peterriver, overviewTab);
        PCLabel = ESPUI.addControl(
            ControlType::Label, "Panel current", F("unknown"), ControlColor::Peterriver, overviewTab);
        PWLabel
            = ESPUI.addControl(ControlType::Label, "Panel power", F("unknown"), ControlColor::Peterriver, overviewTab);

        CTLabel = ESPUI.addControl(
            ControlType::Label, "Controller temperature", F("unknown"), ControlColor::Wetasphalt, overviewTab);
        CSLabel = ESPUI.addControl(
            ControlType::Label, "Controller state", F("unknown"), ControlColor::Wetasphalt, overviewTab);
        ELabel = ESPUI.addControl(ControlType::Label, "Error", F("unknown"), ControlColor::Wetasphalt, overviewTab);

        const uint16_t mqttIPText = ESPUI.addControl(
            ControlType::Text, "Broker IP", "", ControlColor::Sunflower, mqttTab, &GUI::Callback::updateMQTTIP);
        const uint16_t mqttPortNumber = ESPUI.addControl(
            ControlType::Number, "Broker port", "", ControlColor::Sunflower, mqttTab, &GUI::Callback::updateMQTTPort);
        const uint16_t mqttTopic = ESPUI.addControl(
            ControlType::Text, "Topic", "", ControlColor::Sunflower, mqttTab, &GUI::Callback::updateMQTTTopic);
        mqttStatusLabel
            = ESPUI.addControl(ControlType::Label, "Status", F("unknown"), ControlColor::Wetasphalt, mqttTab);

        const uint16_t wifiSSID = ESPUI.addControl(
            ControlType::Text, "SSID", "", ControlColor::Sunflower, wifiTab, &GUI::Callback::updateWifiSSID);
        const uint16_t wifiPassword = ESPUI.addControl(
            ControlType::Text, "Password", "", ControlColor::Sunflower, wifiTab, &GUI::Callback::updateWifiPassword);
        ESPUI.addControl(ControlType::Button, "Connect to WiFi", F("Connect"), ControlColor::Sunflower, wifiTab,
            &GUI::Callback::connectWifiButton);
        wifiStatusLabel
            = ESPUI.addControl(ControlType::Label, "Status", F("unknown"), ControlColor::Wetasphalt, wifiTab);

        // ESPUI.begin("RNG Bridge"); // Change with ui_title
        ESPUI.beginSPIFFS("RNG Bridge"); // If stored in spiffs

        const String mqttPortString(Settings::settings.mqttPort);
        ESPUI.updateText(mqttPortNumber, mqttPortString);
        ESPUI.updateText(mqttTopic, MQTT::topic);
        if (!Settings::firstStart)
        {
            const String ssidString(Settings::settings.ssid);
            const String wifipwString(Settings::settings.password);
            const String mqttIPString(IPAddress(Settings::settings.mqttIP).toString());

            ESPUI.updateText(wifiSSID, ssidString);
            ESPUI.updateText(wifiPassword, wifipwString);
            ESPUI.updateText(mqttIPText, mqttIPString);
        }

        // TODO is this needed?
        if (WiFi.status() == WL_CONNECTED)
        {
            updateWiFiStatus(FPSTR(CONNECTED)); // needs #include "Constants.h"
        }
        else
        {
            updateWiFiStatus(FPSTR(DISCONNECTED)); // needs #include "Constants.h"
        }
    }

    void update(const uint8_t charge, const float batteryVoltage, const float batteryCurrent,
        const int8_t batteryTemperature, const int8_t controllerTemperature, const float laodVoltage,
        const float laodCurrent, const int16_t loadPower, const float panelVoltage, const float panelCurrent,
        const int16_t panelPower, const String& charginStateString, const int32_t errorState)
    {
        ESPUI.print(chargeLabel, String(charge) + " %");
        ESPUI.print(BVLabel, String(batteryVoltage) + " V");
        ESPUI.print(BCLabel, String(batteryCurrent) + " A");
        ESPUI.print(BTLabel, String(batteryTemperature) + " °C");
        ESPUI.print(CTLabel, String(controllerTemperature) + " °C");
        ESPUI.print(LVLabel, String(laodVoltage) + " V");
        ESPUI.print(LCLabel, String(laodCurrent) + " A");

        delayedUpdate.once_ms(
            100, [loadPower, panelVoltage, panelCurrent, panelPower, charginStateString, errorState]() {
                ESPUI.print(LWLabel, String(loadPower) + " W");
                ESPUI.print(PVLabel, String(panelVoltage) + " V");
                ESPUI.print(PCLabel, String(panelCurrent) + " A");
                ESPUI.print(PWLabel, String(panelPower) + " W");
                ESPUI.print(CSLabel, charginStateString);
                ESPUI.print(ELabel, "E" + String(errorState));
            });
    }

    void updateWiFiStatus(const String& status) { ESPUI.updateLabel(wifiStatusLabel, status); }

    void updateMQTTStatus(const String& status) { ESPUI.updateLabel(mqttStatusLabel, status); }

    size_t clients() { return ESPUI.ws->count(); }

    int chargeLabel;
    int BVLabel;
    int BCLabel;
    int BTLabel;
    int CTLabel;
    int LVLabel;
    int LCLabel;
    int LWLabel;
    int PVLabel;
    int PCLabel;
    int PWLabel;
    int CSLabel;
    int ELabel;
    int mqttStatusLabel;
    int wifiStatusLabel;
    Ticker delayedUpdate;
} // namespace GUI
