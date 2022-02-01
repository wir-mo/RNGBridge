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
    time_t epoch = timeClient.getEpochTime();
    const bool success = sendPowerData(_powerGeneration, _powerConsumption, _voltage, epoch);

    // Reset counters
    _powerGeneration = 0.0;
    _powerConsumption = 0.0;
    _voltage = 0.0;

    // Update status
    if (success)
    {
        char temp[18];
        const uint8_t currentHour = hour(epoch);
        const uint8_t currentMinute = minute(epoch);
        sprintf_P(temp, PSTR("Sent data (%hhu:%hhu)"), currentHour, currentMinute);
        DEBUGF("[PVO] %s", temp);
        updateStatus(String(temp));
    }
    else
    {
        DEBUGLN(F("[PVO] Could not send power data"));
        updateStatus(F("Could not send power data"));
    }

    // Update NTP time
    timeClient.update();
}

void PVOutput::updateData(
    const double interval, const double powerGeneration, const double powerConsumption, const double voltage)
{
    // Only update if updateInterval is greater than zero, so we don't accidentally send wrong data and the data
    // does not overflow
    if (_updateInterval > 0)
    {
        const double factor = interval / _updateInterval;
        _powerGeneration += factor * powerGeneration;
        _powerConsumption += factor * powerConsumption;
        _voltage += factor * voltage;
    }
}

void PVOutput::start()
{
    DEBUGLN(F("[PVO] Starting"));
    updateStatus(F("Starting"));
    // Try to get the status interval which can't be 0
    const uint8_t interval = getStatusInterval();
    if (interval > 0)
    {
        // Convert minutes to seconds
        _updateInterval = interval * 60;
        // Do time sync now
        syncTime();

        _started = true;

        // Set status running
        DEBUGLN(F("[PVO] Running"));
        updateStatus(F("Running"));
    }
    else
    {
        _started = false;

        // Set status error
        DEBUGLN(F("[PVO] Could not get update interval, retrying"));
        updateStatus(F("Could not get update interval, retrying"));
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
            // Reset counters
            _powerGeneration = 0.0;
            _powerConsumption = 0.0;
            _voltage = 0.0;

            _secondsPassed = 0;
        }
        start();
    }
}

void PVOutput::setListener(StatusListener listener)
{
    _listener = listener;
    if (_listener)
    {
        _listener(_status);
    }
}

bool PVOutput::syncTime()
{
    bool synced = false;
    // Only attempt sync if the WiFi is connected or we will never get out of the while loop
    if (WiFi.isConnected())
    {
        updateStatus(F("Syncing time"));
        DEBUG(F("[PVO] Syncing time"));
        while (!synced)
        {
            // Force update until we have a correct time
            timeClient.forceUpdate();
            synced = year(timeClient.getEpochTime()) > 2019;
            delay(500);
            DEBUG(".");
        }
        DEBUGLN(F("\n[PVO] Synced time"));
    }
    else
    {
        DEBUGLN(F("[PVO] No WiFi for time sync"));
        updateStatus(F("No WiFi for time sync"));
    }
    return synced;
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
    // DEBUGF("[PVO] GET %s, k: %s, i: %d\n", url, apiKey, sysID);
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

bool PVOutput::sendPowerData(
    const int powerGeneration, const int powerConsumption, const double voltage, const time_t dataTime)
{
    // TODO Maybe need to have a Setting for the timezone (+-hours from GMT)
    const int currentYear = year(dataTime);
    const uint8_t currentMonth = month(dataTime);
    const uint8_t currentDay = day(dataTime);
    const uint8_t currentHour = hour(dataTime);
    const uint8_t currentMinute = minute(dataTime);

    // Generate URL with data
    char data[80]; // 44 static + 8 + 4 + 18
    sprintf_P(data, PSTR("/service/r2/addstatus.jsp?d=%04d%02d%02d&t=%02d:%02d&v2=%d&v4=%d&v6=%.1f"), currentYear,
        currentMonth, currentDay, currentHour, currentMinute, powerGeneration, powerConsumption, voltage);

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

void PVOutput::updateStatus(const String& status)
{
    _status = status;
    if (_listener)
    {
        _listener(_status);
    }
}