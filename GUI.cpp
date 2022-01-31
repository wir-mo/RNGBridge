#include "GUI.h"

#include <Updater.h>

#include "Constants.h"
#include "MQTT.h"
#include "PVOutput.h"

// void updateMQTTIP(Control* sender, int)
// {
//     IPAddress ip {};
//     if (ip.fromString(sender->value))
//     {
//         Settings::updateMQTTIP(ip);
//         MQTT::updateIP(ip);
//     }
// }

// void updateMQTTPort(Control* sender, int)
// {
//     const int32_t port = sender->value.toInt();
//     // Only accept positive values
//     if (port > 0)
//     {
//         const uint16_t port16 = port & 0xFFFF;
//         Settings::updateMQTTPort(port16);
//         MQTT::updatePort(port16);
//     }
// }

// void updateMQTTTopic(Control* sender, int)
// {
//     const uint8_t len = sender->value.length();
//     // Only accept string which are 32 characters max
//     if (len > 0 && len < 32)
//     {
//         Settings::updateMQTTTopic(sender->value);
//         MQTT::disconnect();
//         // MQTT::updateTopic();
//     }
// }

// void updateMQTTUsername(Control* sender, int)
// {
//     const uint8_t len = sender->value.length();
//     // Only accept string which are 32 characters max
//     if (len < 32)
//     {
//         Settings::updateMQTTUsername(sender->value);
//         MQTT::disconnect();
//         // MQTT::updateCredentials();
//     }
// }

// void updateMQTTPassword(Control* sender, int)
// {
//     const uint8_t len = sender->value.length();
//     // Only accept string which are 32 characters max
//     if (len < 32)
//     {
//         Settings::updateMQTTPassword(sender->value);
//         MQTT::disconnect();
//         // MQTT::updateCredentials();
//     }
// }

// void updateMQTTEnable(Control*, int type)
// {
//     const bool enabled = type == S_ACTIVE;
//     Settings::settings.mqtt = enabled;
//     Settings::store();
//     MQTT::update();
// }

// void updateWifiSSID(Control* sender, int)
// {
//     const uint8_t len = sender->value.length();
//     if (len > 0 && len < 32)
//     {
//         if (!Settings::updateWifiSsid(sender->value))
//         {
//             updateWiFiStatus(F("Could not store SSID"));
//         }
//         else
//         {
//             updateWiFiStatus(F("Stored SSID"));
//         }
//     }
//     else
//     {
//         updateWiFiStatus(F("SSID length wrong (1-32 chars)"));
//     }
// }

// void updateWifiPassword(Control* sender, int)
// {
//     const uint8_t len = sender->value.length();
//     if (len < 32)
//     {
//         if (!Settings::updateWifiPassword(sender->value))
//         {
//             updateWiFiStatus(F("Could not store password"));
//         }
//         else
//         {
//             updateWiFiStatus(F("Stored password"));
//         }
//     }
//     else
//     {
//         updateWiFiStatus(F("Password length wrong (0-32 chars)"));
//     }
// }

// void updateWifiEnable(Control*, int type)
// {
//     const bool enabled = type == S_ACTIVE;

//     Settings::settings.wifi = enabled;
//     Settings::store();
//     if (enabled)
//     {
//         WIFI::connect();
//     }
//     else
//     {
//         WIFI::createAP();
//     }
// }

// void updateSystemID(Control* sender, int) { Settings::updateSystemID(sender->value.toInt()); }

// void updateAPIKey(Control* sender, int)
// {
//     const uint8_t len = sender->value.length();
//     if (len > 0 && len < 50)
//     {
//         Settings::updateApiKey(sender->value);
//     }
// }

// void updateTimeOffset(Control* sender, int)
// {
//     const long value = sender->value.toInt();
//     // Offsets can only range from -12 to +14 hours
//     if (value >= -12 && value <= 14)
//     {
//         Settings::updateTimeOffset(value);
//         PVOutput::timeClient.setTimeOffset(value * 3600);
//     }
// }

// void updatePVOutputEnable(Control*, int type)
// {
//     const bool enabled = type == S_ACTIVE;
//     Settings::settings.pvOutput = enabled;
//     Settings::store();
//     PVOutput::update();
// }

// void handleOTAUpload(
//     AsyncWebServerRequest* request, String filename, size_t index, uint8_t* data, size_t len, bool final)
// {
//     if (!index)
//     {
//         DEBUGF("UploadStart: %s\n", filename.c_str());
//         // calculate sketch space required for the update
//         const uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
//         if (!Update.begin(maxSketchSpace))
//         {
//             // start with max available size
//             Update.printError(DEBUG_SERIAL);
//         }
//         Update.runAsync(true);
//     }

//     if (len)
//     {
//         Update.write(data, len);
//     }

//     // if the final flag is set then this is the last frame of data
//     if (final)
//     {
//         if (Update.end(true))
//         {
//             // true to set the size to the current progress
//             DEBUGF("Update Success: %ub written\nRebooting...\n", index + len);
//             ESP.restart();
//         }
//         else
//         {
//             Update.printError(DEBUG_SERIAL);
//         }
//     }
// }

void GUI::updateRenogyStatus(const Renogy::Data& data)
{
    _status["b"]["charge"] = data.batteryCharge;
    _status["b"]["voltage"] = data.batteryVoltage;
    _status["b"]["current"] = data.batteryCurrent;
    _status["b"]["temperature"] = data.batteryTemperature;

    _status["l"]["enabled"] = data.loadEnabled;
    _status["l"]["voltage"] = data.loadVoltage;
    _status["l"]["current"] = data.loadCurrent;

    _status["p"]["voltage"] = data.panelVoltage;
    _status["p"]["current"] = data.panelCurrent;

    _status["s"]["state"] = data.chargingState;
    _status["s"]["error"] = data.errorState;
    _status["s"]["temperature"] = data.controllerTemperature;
}

void GUI::updateWiFiStatus(const String& status)
{
    // ESPUI.updateLabel(wifiStatusLabel, status);
}

void GUI::updateMQTTStatus(const String& status)
{
    _status["mqtt"]["status"] = status;
}

void GUI::updatePVOutputStatus(const String& status)
{
    // ESPUI.updateLabel(pvOutputStatusLabel, status);
}

void GUI::updateUptime(const uint32_t uptime)
{
    DEBUG(F("Uptime: "));
    DEBUGLN(uptime);
    _status["uptime"] = uptime;
}

void GUI::updateHeap(const uint32_t heap)
{
    _status["heap"] = heap;
}

void GUI::update()
{
    String serialized = {};
    serialized.reserve(measureJson(_status));
    serializeJson(_status, serialized);
    _networking.sendTestMessage(serialized);
}
