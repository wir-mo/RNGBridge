#include <ESP8266mDNS.h>

#include "Constants.h"
#include "GUI.h"
#include "MQTT.h"
#include "PVOutput.h"
#include "Renogy.h"
#include "Settings.h"

// 60 requests per hour.
// 300 requests per hour in donation mode.

const uint32_t RENOGY_INTERVAL = 2 * 1000; /// The interval in ms at which the renogy data should be read
uint32_t lastRenogy = 0; /// The last time the renogy data was read

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
    const uint32_t time = millis();
    // handle renogy modbus
    // accomodate for overflow
    uint32_t delta;
    if (time < lastRenogy)
    {
        delta = std::numeric_limits<uint32_t>::max() - lastRenogy + time;
    }
    else
    {
        delta = time - lastRenogy;
    }
    if (delta >= RENOGY_INTERVAL)
    {
        lastRenogy = time;
        // Read and process data every 2 seconds
        Renogy::Callback::readAndProcessData(delta);
    }

    // handle dns
    MDNS.update();
    // handle wifi or whatever the esp is doing
    yield();
}
