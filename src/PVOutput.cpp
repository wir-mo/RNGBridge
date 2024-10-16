#include "PVOutput.h"

#if defined(ESP32)
#include <Update.h>
#else
#include <ESP8266WiFi.h>
#include <Updater.h>
#include <include/WiFiState.h>
#endif

const char* PVOutput::HOST PROGMEM = "pvoutput.org";

void PVOutput::sendData()
{
    // Send power data
    struct tm time = _time.getTmTime();
    const bool success = sendPowerData(
        _energyGeneration, _powerGeneration, _energyConsumption, _powerConsumption, _temperature, _voltage, time);

    // Update status
    if (success)
    {
        char temp[18];
        const uint8_t currentHour = time.tm_hour;
        const uint8_t currentMinute = time.tm_min;
        sprintf_P(temp, PSTR("Sent data (%02d:%02d)"), currentHour, currentMinute);
        RNG_DEBUGF("[PVO] %s", temp);
        notify(String(temp));
    }
    else
    {
        RNG_DEBUGLN(F("[PVO] Could not send power data"));
        notify(F("Could not send power data"));
    }
}

void PVOutput::updateData(const Renogy::Data& data)
{
    if (_initial)
    {
        _initial = false;
        _powerGeneration = data.panelCurrent * data.panelVoltage;
        _powerConsumption = data.loadCurrent * data.loadVoltage;
        _temperature = data.batteryTemperature;
        _voltage = data.batteryVoltage;
        return;
    }

    _powerGeneration += data.panelCurrent * data.panelVoltage;
    _powerConsumption += data.loadCurrent * data.loadVoltage;
    _energyGeneration = data.generation;
    _energyConsumption = data.consumption;
    _temperature += data.batteryTemperature;
    _voltage += data.batteryVoltage;

    // String debug = "+";
    // debug += _powerGeneration;
    // debug += "W(";
    // debug += _energyGeneration;
    // debug += "Wh), -";
    // debug += _powerConsumption;
    // debug += "W(";
    // debug += _energyConsumption;
    // debug += "Wh), ";
    // debug += _voltage;
    // debug += "V, ";
    // debug += _temperature;
    // debug += "°C";
    // notify(debug);
}

void PVOutput::start()
{
    RNG_DEBUGLN(F("[PVO] Starting"));
    notify(F("Starting"));
    // Try to get the status interval which can't be 0
    const uint8_t interval = getStatusInterval();
    if (interval > 0)
    {
        _started = true;

        // Convert minutes to seconds
        _updateInterval = interval * 60;

        // Set status running
        RNG_DEBUGLN(F("[PVO] Running"));
        notify(F("Running"));
    }
    else
    {
        _started = false;

        // Set status error
        RNG_DEBUGLN(F("[PVO] Could not get update interval, retrying"));
        notify(F("Could not get update interval, retrying"));
    }
}

void PVOutput::loop()
{
    if (_started)
    {
        ++_secondsPassed;
        if (_secondsPassed >= _updateInterval)
        {
            _secondsPassed = 0;
            sendData();
        }
    }
    else
    {
        if (_secondsPassed)
        {
            _secondsPassed = 0;
        }
        start();
    }
}

bool PVOutput::httpsGET(const String& url, const bool rateLimit)
{
    // Delegate
    return httpsGET(url.c_str(), rateLimit);
};

bool PVOutput::httpsGET(const char* url, const bool rateLimit)
{
    // Delegate
    return httpsGET(client, url, _config.apiKey.c_str(), _config.systemId, rateLimit);
};

bool PVOutput::httpsGET(
    WiFiClientSecure& client, const char* url, const char* apiKey, const uint32_t sysID, const bool rateLimit)
{
    // RNG_DEBUGF("[PVO] GET %s, k: %s, i: %d\n", url, apiKey, sysID);
    // Try to connect to server
    const bool connected = client.connect(HOST, 443);
    if (connected)
    {
        // Do GET request
        client.print(F("GET "));
        client.print(url);
        client.println(F(" HTTP/1.1"));

        client.print(F("Host: "));
        client.println(HOST);

        client.println(F("User-Agent: ESP8266/1.0"));
        client.println(F("Connection: close"));
        client.println(F("Accept: */*"));

        if (rateLimit)
        {
            client.println(F("X-Rate-Limit: 1"));
        }

        // Append API Key and System ID to Header
        client.print(F("X-Pvoutput-Apikey: "));
        client.println(apiKey);
        client.print(F("X-Pvoutput-SystemId: "));
        client.println(sysID);

        // No content needed for all requests to PVOutput
        client.println(F("Content-Length: 0"));
        client.println();
        client.println();
        delay(10);
    }
    return connected;
};

bool PVOutput::sendPowerData(const int16_t energyGeneration, const uint16_t powerGeneration,
    const int16_t energyConsumption, const uint16_t powerConsumption, const double temperature, const double voltage,
    const tm& tm)
{
    const int currentYear = tm.tm_year;
    const uint8_t currentMonth = tm.tm_mon;
    const uint8_t currentDay = tm.tm_mday;
    const uint8_t currentHour = tm.tm_hour;
    const uint8_t currentMinute = tm.tm_min;

    // v1 Energy Generation Wh (10000)
    // v2 Power Generation W (2000)
    // v3 Energy Consumption Wh (10000)
    // v4 Power Consumption W (2000)
    // v5 Temperature °C (23.4)
    // v6 Voltage V (239.2)
    // c1 Cumulative Flag (1)
    // n Net Flag (1)

    // Generate URL with data
    char data[128]; // 56 static + 8 + 4 + 36
    sprintf_P(data,
        PSTR("/service/r2/addstatus.jsp?d=%04d%02d%02d&t=%02d:%02d&v1=%d&v2=%d&v3=%d&v4=%d&v5=%.1f&v6=%.1f"),
        currentYear, currentMonth, currentDay, currentHour, currentMinute, energyGeneration, powerGeneration,
        energyConsumption, powerConsumption, temperature, voltage);

    // Make request
    bool success = httpsGET(data);

    // Release HEAP
    client.stop();
    return success;
};

uint16_t PVOutput::getRateLimit()
{
    uint16_t limit = 0;
    // Make request
    if (httpsGET(F("/service/r2/getstatus.jsp"), true))
    {
        // if (client.find("X-Rate-Limit-Remaining: ")) {
        //   client.parseInt()
        // }
        // Find rate limit
        if (client.find("X-Rate-Limit-Limit: "))
        {
            // Parse rate limit
            limit = client.parseInt();
        }
        // if (client.find("X-Rate-Limit-Reset: ")) {
        //   client.parseInt()
        // }
    }
    // Release HEAP
    client.stop();
    return limit;
}

uint8_t PVOutput::getStatusInterval()
{
    uint8_t interval = 0;
    // Make request
    if (httpsGET(F("/service/r2/getsystem.jsp")))
    {
        // Remove Header and find Body
        if (client.find("\r\n\r\n"))
        {
            // Data:
            // 125Small island,200,,2,100,Protein P-M100-36P,1,1000,
            // Edecoa 1000W-12V,S,NaN,No,,NaN,NaN,5;;0

            // 1 Name, // text
            // 2 size, // number in watts
            // 3 postcode, // number
            // 4 num of panels, // number
            // 5 panel power, // watts
            // 6 panel brand, // text
            // 7 num of inverters, // number
            // 8 inverter power, // watts
            // 9 inverter brand, // text
            // 10 orientation, // text
            // 11 array tilt, // decimal in degrees
            // 12 shade, // text
            // 13 install date, //yyyymmdd
            // 14 latitude, decimal
            // 15 longitude, decimal
            // 16 status interval, number in minutes
            // 17... don't care about the rest

            // skip 15 commas
            client.find(',');
            client.find(',');
            client.find(',');
            client.find(',');
            client.find(',');
            client.find(',');
            client.find(',');
            client.find(',');
            client.find(',');
            client.find(',');
            client.find(',');
            client.find(',');
            client.find(',');
            client.find(',');
            client.find(',');
            interval = client.parseInt();
        }
    }
    // Release HEAP
    client.stop();
    return interval;
}
