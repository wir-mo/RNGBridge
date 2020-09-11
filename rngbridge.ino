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
    Serial.begin(9600);
    delay(4000);
    // Signal startup
    pinMode(LED_BUILTIN, OUTPUT); // Initialize the LED_BUILTIN pin as an output
    digitalWrite(LED_BUILTIN, LOW);

    // Setup eeprom and check if this is the first start
    Settings::begin();

    // ui setup
    Serial.println(F("GUI"));
    GUI::setup();

    Serial.println(F("WIFI"));
    WIFI::setup();
    if (Settings::settings.wifi)
    {
        // Try to connect or else create AP
        Serial.println(F("Client"));
        Serial.println(Settings::settings.ssid);
        Serial.println(Settings::settings.password);
        WIFI::connectToAP(Settings::settings.ssid, Settings::settings.password);
    }
    else
    {
        Serial.println(F("AP"));
        WIFI::createAP();
    }

    Serial.println(F("MQTT"));
    MQTT::setup();
    Serial.println(F("Update"));
    MQTT::update();

    Serial.println(F("PV"));
    PVOutput::setup();
    Serial.println(F("Update"));
    PVOutput::update();

    Serial.println(F("Renogy"));
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
