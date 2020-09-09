#include <ESP8266mDNS.h>

#include "Constants.h"

#ifdef HAVE_GUI
#include "GUI.h"
#endif

#include "MQTT.h"
#include "PVOutput.h"
#include "Renogy.h"
#include "Settings.h"

// 60 requests per hour.
// 300 requests per hour in donation mode.

void setup()
{
    WIFI::setup();

    // setup eeprom and check if this is the first start
    Settings::begin();

    if (Settings::firstStart)
    {
        // We can directly start the AP since we don't have a configuration
        WIFI::createAP();

        // Load default settings
        Settings::settings.mqttPort = 1883;
        Settings::updateMQTTTopic(F("/rng"));
    }
    else
    {
        // Try to connect or else create AP
        const String ssid = String(Settings::settings.ssid);
        const String password = String(Settings::settings.password);
        WIFI::connect(ssid, password);
    }

#ifdef HAVE_GUI
    // ui setup
    GUI::setup();
#endif

    // Setup DNS so we don't have to find and type the ip address
    MDNS.begin(FPSTR(hostname));

    if (!Settings::firstStart)
    {
        MQTT::setup();
        MQTT::connect();

        PVOutput::setup();
        PVOutput::start();
    }

    Renogy::setup();

    // Signal setup complete
    // TODO Make usage of external LED, that is not connected to the TX line
    pinMode(LED_BUILTIN, OUTPUT); // Initialize the LED_BUILTIN pin as an output
    digitalWrite(LED_BUILTIN, HIGH);
    blinkLED();
    delay(100);
    blinkLED();
    delay(100);
    blinkLED();
}

void loop()
{
    // handle dns
    MDNS.update();
    // handle wifi or whatever the esp is doing
    yield();
}
