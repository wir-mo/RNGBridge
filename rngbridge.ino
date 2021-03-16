#include <ESP8266mDNS.h>

#include "Constants.h"
#include "GUI.h"
#include "MQTT.h"
#include "PVOutput.h"
#include "Renogy.h"
#include "Settings.h"

// 60 requests per hour.
// 300 requests per hour in donation mode.

const uint32_t RENOGY_INTERVAL = 2; /// The interval in s at which the renogy data should be read

uint8_t lastSecond = 0; /// The last seconds value
uint8_t secondsPassedRenogy = 0; /// amount of seconds passed
uint16_t secondsPassedPVOutput = 0; /// amount of seconds passed

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
    const uint8_t currentSecond = (time / 1000) % 60;

    if (currentSecond != lastSecond)
    {
        lastSecond = currentSecond;

        ++secondsPassedRenogy;
        if (secondsPassedRenogy >= RENOGY_INTERVAL)
        {
            secondsPassedRenogy = 0;
            // Read and process data every 2 seconds
            Renogy::Callback::readAndProcessData(2000);
        }

        if (WIFI::connected)
        {
            if (Settings::settings.pvOutput)
            {
                if (PVOutput::started)
                {
                    ++secondsPassedPVOutput;
                    if (secondsPassedPVOutput >= PVOutput::_updateInterval)
                    {
                        secondsPassedPVOutput = 0;
                        PVOutput::Callback::sendData();
                    }
                }
                else
                {
                    if (secondsPassedPVOutput)
                    {
                        // Reset counters
                        PVOutput::_powerGeneration = 0.0;
                        PVOutput::_powerConsumption = 0.0;
                        PVOutput::_panelVoltage = 0.0;

                        secondsPassedPVOutput = 0;
                    }
                    PVOutput::start();
                }
            }

            if (Settings::settings.mqtt && !MQTT::connected)
            {
                MQTT::connect();
            }
        }
        else
        {
            if (Settings::settings.wifi)
            {
                WIFI::connect();
            }
        }

        // handle dns
        MDNS.update();
    }

    // handle wifi or whatever the esp is doing
    yield();
    // delay(200);
}
