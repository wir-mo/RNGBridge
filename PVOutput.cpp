#include "PVOutput.h"

#include "GUI.h"
#include "Settings.h"

namespace PVOutput
{
    namespace Callback
    {
        void sendData()
        {
            // Send power data
            const bool success = sendPowerData(_energyGeneration, _powerGeneration, _energyConsumption,
                _powerConsumption, _temperature, _voltage, timeClient.getEpochTime());

            // Update GUI
            if (success)
            {
                GUI::updatePVOutputStatus(F("Sent power data"));
            }
            else
            {
                GUI::updatePVOutputStatus(F("Could not send power data"));
            }

            // Update NTP time
            timeClient.update();
        }

        void updateData(const int16_t energyGeneration, const double powerGeneration, const int16_t energyConsumption,
            const double powerConsumption, const double temperature, const double voltage)
        {
            if (_initial)
            {
                _initial = false;
                _powerGeneration = powerGeneration;
                _powerConsumption = powerConsumption;
                _temperature = temperature;
                _voltage = voltage;
                return;
            }

            _energyGeneration = energyGeneration;
            _energyConsumption = energyConsumption;
            _powerGeneration += powerGeneration;
            _powerConsumption += powerConsumption;
            _temperature += temperature;
            _voltage += voltage;
        }
    } // namespace Callback

    void setup()
    {
        // We need to reduce the buffer sizes or we get issues with HEAP
        client.setBufferSizes(4096, 512);
        // Don't want to use Cert Store or Fingerprint cause they need to be updated
        client.setInsecure();
        // Set time offset, convert hours to seconds
        timeClient.setTimeOffset(Settings::settings.timeOffset * 3600);
        // Setup time client
        timeClient.begin();
    }

    void start()
    {
        GUI::updatePVOutputStatus(F("Starting"));
        // Try to get the status interval which can't be 0
        const uint8_t interval = getStatusInterval();
        if (interval > 0)
        {
            // Convert minutes to seconds
            _updateInterval = interval * 60;
            // Do time sync now
            syncTime();

            GUI::updatePVOutputStatus(F("Running"));

            started = true;
        }
        else
        {
            started = false;

            // Set status error
            GUI::updatePVOutputStatus(F("Could not get update interval, retrying in 5s"));
        }
    }

    void stop()
    {
        started = false;
        _updateInterval = 0;
    }

    void update()
    {
        // If PVOutput is enabled by settings start the "service" or else stop it
        if (Settings::settings.pvOutput)
        {
            start();
        }
        else
        {
            stop();
            GUI::updatePVOutputStatus(F("Stopped"));
        }
    }

    bool syncTime()
    {
        bool synced = false;
        // Only attempt sync if the WiFi is connected or we will never get out of the while loop
        if (WiFi.isConnected())
        {
            GUI::updatePVOutputStatus(F("Syncing time"));
            while (!synced)
            {
                // Force update until we have a correct time
                timeClient.forceUpdate();
                synced = year(timeClient.getEpochTime()) > 2019;
                delay(500);
            }
        }
        else
        {
            GUI::updatePVOutputStatus(F("No WiFi for time sync"));
        }
        return synced;
    }

    bool httpsGET(const String& url, const bool rateLimit)
    {
        // Delegate
        return httpsGET(url.c_str(), rateLimit);
    };

    bool httpsGET(const char* url, const bool rateLimit)
    {
        // Delegate
        return httpsGET(client, url, Settings::settings.apiKey, Settings::settings.systemID, rateLimit);
    };

    bool httpsGET(
        WiFiClientSecure& client, const char* url, const char* apiKey, const uint32_t sysID, const bool rateLimit)
    {
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

    bool sendPowerData(const int16_t energyGeneration, const double powerGeneration, const int16_t energyConsumption,
        const double powerConsumption, const double temperature, const double voltage, const time_t dataTime)
    {
        // TODO Maybe need to have a Setting for the timezone (+-hours from GMT)
        const int currentYear = year(dataTime);
        const uint8_t currentMonth = month(dataTime);
        const uint8_t currentDay = day(dataTime);
        const uint8_t currentHour = hour(dataTime);
        const uint8_t currentMinute = minute(dataTime);

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

    uint16_t getRateLimit()
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

    uint8_t getStatusInterval()
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

    const char* HOST PROGMEM = "pvoutput.org";

    WiFiClientSecure client;
    WiFiUDP ntpUDP;
    NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", 0, 60000);

    int16_t _energyGeneration = 0;
    int16_t _energyConsumption = 0;
    ApproxRollingAverage<double, 60 / 2> _powerGeneration
        = ApproxRollingAverage<double, 60 / 2>(); /// Internal average for power generation in W
    ApproxRollingAverage<double, 60 / 2> _powerConsumption
        = ApproxRollingAverage<double, 60 / 2>(); /// Internal average for power consumption in W
    ApproxRollingAverage<double, 60 / 2> _voltage
        = ApproxRollingAverage<double, 60 / 2>(); /// Internal average for voltage
    ApproxRollingAverage<double, 60 / 2> _temperature
        = ApproxRollingAverage<double, 60 / 2>(); /// Internal average for temperature
    int _updateInterval = 0;

    bool _initial = true;
    bool started = false;
} // namespace PVOutput