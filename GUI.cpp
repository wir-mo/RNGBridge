#include <Updater.h>

#include "Constants.h"
#include "GUI.h"
#include "MQTT.h"
#include "PVOutput.h"
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
            if (len > 0 && len < 32)
            {
                Settings::updateMQTTTopic(sender->value);
                MQTT::updateTopic();
            }
        }

        void updateMQTTUsername(Control* sender, int type)
        {
            const uint8_t len = sender->value.length();
            // Only accept string which are 32 characters max
            if (len < 32)
            {
                Settings::updateMQTTUsername(sender->value);
                MQTT::updateCredentials();
            }
        }

        void updateMQTTPassword(Control* sender, int type)
        {
            const uint8_t len = sender->value.length();
            // Only accept string which are 32 characters max
            if (len < 32)
            {
                Settings::updateMQTTPassword(sender->value);
                MQTT::updateCredentials();
            }
        }

        void updateMQTTEnable(Control* sender, int type)
        {
            const bool enabled = type == S_ACTIVE;
            Settings::settings.mqtt = enabled;
            Settings::store();
            MQTT::update();
        }

        void updateWifiSSID(Control* sender, int type)
        {
            const uint8_t len = sender->value.length();
            if (len > 0 && len < 32)
            {
                if (!Settings::updateWifiSsid(sender->value))
                {
                    updateWiFiStatus(F("Could not store SSID"));
                }
                else
                {
                    updateWiFiStatus(F("Stored SSID"));
                }
            }
            else
            {
                updateWiFiStatus(F("SSID length wrong (1-32 chars)"));
            }
        }

        void updateWifiPassword(Control* sender, int type)
        {
            const uint8_t len = sender->value.length();
            if (len >= 0 && len < 32)
            {
                if (!Settings::updateWifiPassword(sender->value))
                {
                    updateWiFiStatus(F("Could not store password"));
                }
                else
                {
                    updateWiFiStatus(F("Stored password"));
                }
            }
            else
            {
                updateWiFiStatus(F("Password length wrong (0-32 chars)"));
            }
        }

        void updateWifiEnable(Control* sender, int type)
        {
            const bool enabled = type == S_ACTIVE;

            Settings::settings.wifi = enabled;
            Settings::store();
            if (enabled)
            {
                WIFI::connect();
            }
            else
            {
                WIFI::createAP();
            }
        }

        void updateSystemID(Control* sender, int type) { Settings::updateSystemID(sender->value.toInt()); }

        void updateAPIKey(Control* sender, int type)
        {
            const uint8_t len = sender->value.length();
            if (len > 0 && len < 50)
            {
                Settings::updateApiKey(sender->value);
            }
        }

        void updateTimeOffset(Control* sender, int type)
        {
            const long value = sender->value.toInt();
            // Offsets can only range from -12 to +14 hours
            if (value >= -12 && value <= 14)
            {
                Settings::updateTimeOffset(value);
                PVOutput::timeClient.setTimeOffset(value * 3600);
            }
        }

        void updatePVOutputEnable(Control* sender, int type)
        {
            const bool enabled = type == S_ACTIVE;
            Settings::settings.pvOutput = enabled;
            Settings::store();
            PVOutput::update();
        }

        void handleOTAUpload(
            AsyncWebServerRequest* request, String filename, size_t index, uint8_t* data, size_t len, bool final)
        {
            if (!index)
            {
                Serial.printf("UploadStart: %s\n", filename.c_str());
                // calculate sketch space required for the update
                const uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
                if (!Update.begin(maxSketchSpace))
                {
                    // start with max available size
                    Update.printError(Serial);
                }
                Update.runAsync(true);
            }

            if (len)
            {
                Update.write(data, len);
            }

            // if the final flag is set then this is the last frame of data
            if (final)
            {
                if (Update.end(true))
                {
                    // true to set the size to the current progress
                    Serial.printf("Update Success: %ub written\nRebooting...\n", index + len);
                    ESP.restart();
                }
                else
                {
                    Update.printError(Serial);
                }
            }
        }

    } // namespace Callback

    void setup()
    {
        const uint16_t overviewTab = ESPUI.addControl(ControlType::Tab, "Overview", F("Overview"));
        const uint16_t mqttTab = ESPUI.addControl(ControlType::Tab, "MQTT", F("MQTT"));
        const uint16_t pvoutputTab = ESPUI.addControl(ControlType::Tab, "PVOutput", F("PVOutput"));
        const uint16_t wifiTab = ESPUI.addControl(ControlType::Tab, "WiFi", F("WiFi"));

        // Overview
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

        // MQTT
        const uint16_t mqttIPText = ESPUI.addControl(
            ControlType::Text, "Broker IP", "", ControlColor::Sunflower, mqttTab, &GUI::Callback::updateMQTTIP);
        const uint16_t mqttPortNumber = ESPUI.addControl(
            ControlType::Number, "Broker port", "", ControlColor::Sunflower, mqttTab, &GUI::Callback::updateMQTTPort);
        const uint16_t mqttTopic = ESPUI.addControl(
            ControlType::Text, "Topic", "", ControlColor::Sunflower, mqttTab, &GUI::Callback::updateMQTTTopic);
        const uint16_t mqttUsername = ESPUI.addControl(
            ControlType::Text, "Username", "", ControlColor::Sunflower, mqttTab, &GUI::Callback::updateMQTTUsername);
        const uint16_t mqttPassword = ESPUI.addControl(
            ControlType::Text, "Password", "", ControlColor::Sunflower, mqttTab, &GUI::Callback::updateMQTTPassword);
        const uint16_t mqttEnabled = ESPUI.addControl(
            ControlType::Switcher, "Enable", "", ControlColor::Sunflower, mqttTab, &GUI::Callback::updateMQTTEnable);
        mqttStatusLabel
            = ESPUI.addControl(ControlType::Label, "Status", F("unknown"), ControlColor::Wetasphalt, mqttTab);

        // PVOutput
        const uint16_t pvoutputSysId = ESPUI.addControl(ControlType::Number, "System ID", "0", ControlColor::Sunflower,
            pvoutputTab, &GUI::Callback::updateSystemID);
        const uint16_t pvoutputAPIKey = ESPUI.addControl(ControlType::Text, "API Key", F("unknown"),
            ControlColor::Sunflower, pvoutputTab, &GUI::Callback::updateAPIKey);
        const uint16_t pvoutputTimeOffset = ESPUI.addControl(ControlType::Number, "Time offset from UTC", "0",
            ControlColor::Sunflower, pvoutputTab, &GUI::Callback::updateTimeOffset);
        const uint16_t pvoutputEnabled = ESPUI.addControl(ControlType::Switcher, "Enable", "", ControlColor::Sunflower,
            pvoutputTab, &GUI::Callback::updatePVOutputEnable);
        pvOutputStatusLabel
            = ESPUI.addControl(ControlType::Label, "Status", F("unknown"), ControlColor::Wetasphalt, pvoutputTab);

        // WiFi
        const uint16_t wifiSSID = ESPUI.addControl(
            ControlType::Text, "SSID", "", ControlColor::Sunflower, wifiTab, &GUI::Callback::updateWifiSSID);
        const uint16_t wifiPassword = ESPUI.addControl(
            ControlType::Text, "Password", "", ControlColor::Sunflower, wifiTab, &GUI::Callback::updateWifiPassword);
        const uint16_t wifiEnabled = ESPUI.addControl(
            ControlType::Switcher, "Enable", "", ControlColor::Sunflower, wifiTab, &GUI::Callback::updateWifiEnable);
        wifiStatusLabel
            = ESPUI.addControl(ControlType::Label, "Status", F("unknown"), ControlColor::Wetasphalt, wifiTab);

        ESPUI.begin("RNG Bridge"); // Change with ui_title

        ESPUI.updateNumber(mqttPortNumber, Settings::settings.mqttPort);
        ESPUI.updateText(mqttTopic, Settings::settings.topic);
        ESPUI.updateText(mqttUsername, Settings::settings.mqttUser);
        ESPUI.updateText(mqttPassword, Settings::settings.mqttPass);
        ESPUI.updateSwitcher(mqttEnabled, Settings::settings.mqtt);

        ESPUI.updateSwitcher(wifiEnabled, Settings::settings.wifi);

        ESPUI.updateSwitcher(pvoutputTimeOffset, Settings::settings.timeOffset);
        ESPUI.updateSwitcher(pvoutputEnabled, Settings::settings.pvOutput);
        if (!Settings::firstStart)
        {
            ESPUI.updateText(wifiSSID, Settings::settings.ssid);
            ESPUI.updateText(wifiPassword, Settings::settings.password);
            ESPUI.updateText(mqttIPText, IPAddress(Settings::settings.mqttIP).toString());

            ESPUI.updateText(pvoutputAPIKey, Settings::settings.apiKey);
            ESPUI.updateNumber(pvoutputSysId, Settings::settings.systemID);
        }

#ifdef OTA
        ESPUI.server->on(
            "/ota", HTTP_POST, [](AsyncWebServerRequest* request) { request->send(200); }, Callback::handleOTAUpload);

        ESPUI.server->on("/ota", HTTP_GET, [](AsyncWebServerRequest* request) {
            AsyncWebServerResponse* response = request->beginResponse_P(200, "text/html", OTA_INDEX);
            request->send(response);
        });
#endif // OTA
    }

    void update(const uint8_t charge, const float batteryVoltage, const float batteryCurrent,
        const int8_t batteryTemperature, const int8_t controllerTemperature, const float laodVoltage,
        const float laodCurrent, const int16_t loadPower, const float panelVoltage, const float panelCurrent,
        const int16_t panelPower, const int8_t chargingState, const int32_t errorState)
    {
        ESPUI.print(chargeLabel, String(charge) + " %");
        ESPUI.print(BVLabel, String(batteryVoltage) + " V");
        ESPUI.print(BCLabel, String(batteryCurrent) + " A");
        ESPUI.print(BTLabel, String(batteryTemperature) + " °C");
        ESPUI.print(CTLabel, String(controllerTemperature) + " °C");
        ESPUI.print(LVLabel, String(laodVoltage) + " V");
        ESPUI.print(LCLabel, String(laodCurrent) + " A");

        String charginStateString;
        switch (chargingState)
        {
        case 0x00:
            charginStateString = F("inactive");
            break;
        case 0x01:
            charginStateString = F("active");
            break;
        case 0x02:
            charginStateString = F("mppt");
            break;
        case 0x03:
            charginStateString = F("equalize");
            break;
        case 0x04:
            charginStateString = F("boost");
            break;
        case 0x05:
            charginStateString = F("float");
            break;
        case 0x06:
            charginStateString = F("current limiting");
            break;

        default:
            charginStateString = F("unknown");
            break;
        }

        // See Renogy.cpp for more info on this
        uint8_t controllerError = 0;
        if (errorState & 0xFFFF0000)
        {
            if (errorState & 0x40000000)
            {
                controllerError = 15;
            }
            else if (errorState & 0x20000000)
            {
                controllerError = 14;
            }
            else if (errorState & 0x10000000)
            {
                controllerError = 13;
            }
            else if (errorState & 0x8000000)
            {
                controllerError = 12;
            }
            else if (errorState & 0x4000000)
            {
                controllerError = 11;
            }
            else if (errorState & 0x2000000)
            {
                controllerError = 10;
            }
            else if (errorState & 0x1000000)
            {
                controllerError = 9;
            }
            else if (errorState & 0x800000)
            {
                controllerError = 8;
            }
            else if (errorState & 0x400000)
            {
                controllerError = 7;
            }
            else if (errorState & 0x200000)
            {
                controllerError = 6;
            }
            else if (errorState & 0x100000)
            {
                controllerError = 5;
            }
            else if (errorState & 0x80000)
            {
                controllerError = 4;
            }
            else if (errorState & 0x40000)
            {
                controllerError = 3;
            }
            else if (errorState & 0x20000)
            {
                controllerError = 2;
            }
            else if (errorState & 0x10000)
            {
                controllerError = 1;
            }
        }

        delay(250); // Try delaying to send data (maybe increase to 500)
        ESPUI.print(LWLabel, String(loadPower) + " W");
        ESPUI.print(PVLabel, String(panelVoltage) + " V");
        ESPUI.print(PCLabel, String(panelCurrent) + " A");
        ESPUI.print(PWLabel, String(panelPower) + " W");
        ESPUI.print(CSLabel, charginStateString);
        ESPUI.print(ELabel, "E" + String(controllerError));
    }

    void updateWiFiStatus(const String& status) { ESPUI.updateLabel(wifiStatusLabel, status); }

    void updateMQTTStatus(const String& status) { ESPUI.updateLabel(mqttStatusLabel, status); }

    void updatePVOutputStatus(const String& status) { ESPUI.updateLabel(pvOutputStatusLabel, status); }

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
    int pvOutputStatusLabel;
} // namespace GUI
