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

        if (firstStart)
        {
            initSettings();
        }
        else
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

    bool updateSystemID(const uint32_t systemID)
    {
        settings.systemID = systemID;
        return store();
    }

    bool updateApiKey(const String& apiKey)
    {
        apiKey.toCharArray(settings.apiKey, 50);
        return store();
    }

    bool updateWifiPassword(const String& password)
    {
        password.toCharArray(settings.password, 32);
        return store();
    }

    bool updateWifiSsid(const String& ssid)
    {
        ssid.toCharArray(settings.ssid, 32);
        return store();
    }

    bool initSettings()
    {
        strlcpy(settings.ssid, "", 32);
        strlcpy(settings.password, "", 32);
        strlcpy(settings.topic, "/rng", 32);
        strlcpy(settings.apiKey, "", 50);
        settings.systemID = 0;
        settings.mqttIP = 0;
        settings.mqttPort = 1883;
        settings.wifi = false;
        settings.mqtt = false;
        settings.pvOutput = false;
        return store();
    }

    bool firstStart = false;
    struct Settings settings;
} // namespace Settings