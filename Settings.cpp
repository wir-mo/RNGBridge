#include "Settings.h"

#include <Arduino.h> // Needed for memcpy
#include <ESP_EEPROM.h>

namespace Settings
{
    void begin()
    {
        // The begin() call will find the data previously saved in EEPROM if the same size
        // as was previously committed. If the size is different then the EEEPROM data is cleared.
        // Note that this is not made permanent until you call commit();
        EEPROM.begin(sizeof(Settings));

        firstStart = EEPROM.percentUsed() < 0;

        if (!firstStart)
        {
            load();
        }

        // EEPROM.commitReset(); // "Factory defaults"
    }

    void load() { EEPROM.get(0, settings); }

    bool store()
    {
        EEPROM.put(0, settings);
        return EEPROM.commit();
    }

    bool updateMQTTPort(const uint16_t port)
    {
        settings.mqttPort = port;
        return store();
    }

    bool updateMQTTIP(const IPAddress ip)
    {
        settings.mqttIP = ip;
        return store();
    }

    bool updateMQTTTopic(const String& topic)
    {
        topic.toCharArray(settings.topic, 32);
        return store();
    }

    bool firstStart = false;
    struct Settings settings;
} // namespace Settings