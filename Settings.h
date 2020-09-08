#pragma once

#include <IPAddress.h>
#include <String.h>
#include <stdint.h>

namespace Settings
{
    // Settings stored in eeprom
    struct Settings
    {
        // bool pvOutput;
        // bool mqtt;
        // bool autoReconnect;
        // char apiKey[32];   // 08124bfa21591165ca948b67ddd87152c4e8eabb
        // char systemID[32]; // 72583
        uint32_t mqttIP;
        uint16_t mqttPort;
        char ssid[32];
        char password[32];
        char topic[32];
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

    /**
     * @brief Indicates whether this is a start without configuration
     */
    extern bool firstStart;

    extern struct Settings settings;

} // namespace Settings