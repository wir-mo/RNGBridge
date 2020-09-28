#pragma once

#include <IPAddress.h>
#include <String.h>
#include <stdint.h>

namespace Settings
{
    // Settings stored in eeprom
    struct Settings
    {
        char ssid[32];
        char password[32];
        char topic[32];
        char apiKey[50];
        uint32_t systemID;
        uint32_t mqttIP;
        uint16_t mqttPort;
        int8_t timeOffset;
        bool wifi;
        bool mqtt;
        bool pvOutput;
    };

    /**
     * @brief Start the EEPROM (emulated) to be able to read/write the config
     *
     * Will also load the settings if there are any stored
     */
    extern void begin();

    /**
     * @brief Load the settings
     *
     */
    extern void load();

    /**
     * @brief Store the settings
     *
     * @return true Settings have been stored
     * @return false Settings could not be stored
     */
    extern bool store();

    extern bool updateMQTTPort(const uint16_t port);

    extern bool updateMQTTIP(const IPAddress ip);

    extern bool updateMQTTTopic(const String& topic);

    extern bool updateSystemID(const uint32_t systemID);

    extern bool updateApiKey(const String& apiKey);

    extern bool updateTimeOffset(const int8_t timeOffset);

    extern bool updateWifiPassword(const String& password);

    extern bool updateWifiSsid(const String& ssid);

    extern bool initSettings();

    /**
     * @brief Indicates whether this is a start without configuration
     */
    extern bool firstStart;

    extern struct Settings settings;

} // namespace Settings