#include <DNSServer.h>
#include <ESP8266mDNS.h>

#include "Constants.h"
#include "GUI.h"
#include "MQTT.h"
#include "PVOutput.h"
#include "Renogy.h"
#include "Settings.h"
// 60 requests per hour.
// 300 requests per hour in donation mode.

// DNS server for captive portal
DNSServer dnsServer;
bool captiveDNS = false; /// Is the captive portal enabled?

const uint32_t RENOGY_INTERVAL = 2; /// The interval in s at which the renogy data should be read

uint8_t lastSecond = 0; /// The last seconds value
uint8_t secondsPassedRenogy = 0; /// amount of seconds passed
uint16_t secondsPassedPVOutput = 0; /// amount of seconds passed

/**
 * @brief Check if the given string is an ip address
 *
 * @param str String to check
 * @return true If the string is an ip
 * @return false If the string is not an ip
 */
bool isIp(const String& str)
{
    for (size_t i = 0; i < str.length(); i++)
    {
        int c = str.charAt(i);
        if (c != '.' && (c < '0' || c > '9'))
        {
            return false;
        }
    }
    return true;
}

/**
 * @brief Callback used for captive portal webserver
 *
 * @param request
 * @return true
 * @return false
 */
bool captivePortal(AsyncWebServerRequest* request)
{
    if (ON_STA_FILTER(request))
    {
        return false; // only serve captive in AP mode
    }
    String hostH;
    if (!request->hasHeader("Host"))
    {
        return false;
    }
    hostH = request->getHeader("Host")->value();

    if (!isIp(hostH) && hostH.indexOf(FPSTR(HOSTNAME)) < 0)
    {
        // Serial1.println(F("Captive portal"));
        AsyncWebServerResponse* response = request->beginResponse(302);
        response->addHeader(F("Location"), F("http://192.168.1.1"));
        request->send(response);
        return true;
    }
    return false;
}

/**
 * @brief Initialize the captive portal, which opens up the 'login to this network' site, whenever a WiFi client
 * connects
 */
void initCaptivePortal()
{
    // auto handleCaptivePortal = [](AsyncWebServerRequest* request) { captivePortal(request); };
    // Android captive portal. Maybe not needed. Might be handled by notFound handler.
    // ESPUI.server->on("/generate_204", HTTP_GET, handleCaptivePortal);
    // Microsoft captive portal. Maybe not needed. Might be handled by notFound handler.
    // ESPUI.server->on("/fwlink", HTTP_GET, handleCaptivePortal);
    ESPUI.server->onNotFound(captivePortal);

    // ESPUI.server->begin();
    dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
}

void setup()
{
    // Signal startup
    pinMode(LED_BUILTIN, OUTPUT); // Initialize the LED_BUILTIN pin as an output
    digitalWrite(LED_BUILTIN, LOW);

    // Setup eeprom and check if this is the first start
    Settings::begin();

    // UI setup
    GUI::setup();

    // Captive portal so that noone has to enter the IP
    initCaptivePortal();

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
                        PVOutput::_voltage = 0.0;

                        secondsPassedPVOutput = 0;
                    }
                    PVOutput::start();
                }
            }

            if (Settings::settings.mqtt)
            {
                if (!MQTT::mqtt.connected())
                {
                    MQTT::connect();
                }
                MQTT::mqtt.loop();
            }

            if (captiveDNS)
            {
                captiveDNS = false;
                dnsServer.stop();
            }
        }
        else
        {
            if (Settings::settings.wifi)
            {
                WIFI::connect();
            }
            else
            {
                if (!captiveDNS)
                {
                    captiveDNS = true;
                    dnsServer.start(53, "*", WiFi.softAPIP());
                }
                // handle DNS
                dnsServer.processNextRequest();
            }
        }

        // handle dns
        MDNS.update();
    }

    // handle wifi or whatever the esp is doing
    yield();
    // delay(200);
}
