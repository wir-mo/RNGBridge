#include <ESP8266mDNS.h>

#include "Constants.h"
#include "GUI.h"
#include "MQTT.h"
#include "PVOutput.h"
#include "Renogy.h"
#include "Settings.h"

// 60 requests per hour.
// 300 requests per hour in donation mode.

void setup()
{
    // Signal startup
    pinMode(LED_BUILTIN, OUTPUT); // Initialize the LED_BUILTIN pin as an output
    digitalWrite(LED_BUILTIN, LOW);

    // Setup eeprom and check if this is the first start
    Settings::begin();

    // UI setup
    GUI::setup();

    // WiFi setup
    WIFI::setup();
    if (Settings::settings.wifi)
    {
        // Try to connect or else create AP
        if (!WIFI::connectToAP(Settings::settings.ssid, Settings::settings.password))
        {
            WIFI::createAP();
        }
    }
    else
    {
        WIFI::createAP();
    }

    // MQTT setup
    MQTT::setup();
    MQTT::update();

    // PVOutput setup
    PVOutput::setup();
    PVOutput::update();

    // Final modbus/renogy setup
    Renogy::setup();

    // Signal setup done
    digitalWrite(LED_BUILTIN, HIGH);
}

void loop()
{
    // handle dns
    MDNS.update();
    // handle wifi or whatever the esp is doing
    yield();
}
