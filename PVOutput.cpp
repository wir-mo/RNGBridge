#include "PVOutput.h"

#include "GUI.h"
#include "Settings.h"

namespace PVOutput
{
    namespace Callback
    {
        void sendData()
        {
            const bool success = sendPowerData(_powerGeneration, _powerConsumption, 0.0, timeClient.getEpochTime());
            _powerGeneration = 0.0;
            _powerConsumption = 0.0;
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

        void updateData(const uint8_t interval, const double powerGeneration, const double powerConsumption,
            const double panelVoltage)
        {
            if (_updateInterval > 0)
            {
                _powerGeneration += ((double)interval / _updateInterval) * powerGeneration;
                _powerConsumption += ((double)interval / _updateInterval) * powerConsumption;
                _panelVoltage += ((double)interval / _updateInterval) * panelVoltage;
            }
        }
    } // namespace Callback

    void setup()
    {
        client.setBufferSizes(4096, 512);
        client.setInsecure();

        // Setup time client and force time sync
        timeClient.begin();
    }

    void start()
    {
        const uint8_t interval = getStatusInterval();
        if (interval > 0)
        {
            // Send data every interval minutes
            _updateInterval = interval * 60;
            syncTime();
            updateTimer.attach_scheduled(_updateInterval, PVOutput::Callback::sendData);
            GUI::updatePVOutputStatus(F("Running"));
        }
        else
        {
            // Set status error
            GUI::updatePVOutputStatus(F("Could not get update interval"));
        }
    }

    void stop() { updateTimer.detach(); }

    void update()
    {
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
        if (WiFi.isConnected())
        {
            GUI::updatePVOutputStatus(F("Syncing time"));
            while (!synced)
            {
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

    bool httpsGET(const String& url, const bool rateLimit) { return httpsGET(url.c_str(), rateLimit); };

    bool httpsGET(const char* url, const bool rateLimit)
    {
        return httpsGET(client, url, Settings::settings.apiKey, Settings::settings.systemID, rateLimit);
    };

    bool httpsGET(
        WiFiClientSecure& client, const char* url, const char* apiKey, const uint32_t sysID, const bool rateLimit)
    {
        const bool connected = client.connect(HOST, 443);
        if (connected)
        {
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

            client.print(F("X-Pvoutput-Apikey: "));
            client.println(apiKey);
            client.print(F("X-Pvoutput-SystemId: "));
            client.println(sysID);

            client.println(F("Content-Length: 0"));
            client.println();
            client.println();
            delay(10);
        }
        return connected;
    };

    bool sendPowerData(
        const int powerGeneration, const int powerConsumption, const double voltage, const time_t dataTime)
    {
        // TODO need to add a configurable amount of hours AKA new SETTING
        const int currentYear = year(dataTime);
        const uint8_t currentMonth = month(dataTime);
        const uint8_t currentDay = day(dataTime);
        const uint8_t currentHour = hour(dataTime);
        const uint8_t currentMinute = minute(dataTime);

        char data[86];
        // Format: "d=yyyymmdd&t=hh:mm&v1=xxx&v3=xxx"
        // sprintf(data, "d=%04d%02d%02d&t=%02d:%02d&v1=%.3f&v3=%.3f", currentYear, currentMonth, currentDay,
        // currentHour, currentMinute, energyGeneration, energyConsumption);
        sprintf_P(data, PSTR("/service/r2/addstatus.jsp?d=%04d%02d%02d&t=%02d:%02d&v2=%d&v4=%d&v6=%.1f"), currentYear,
            currentMonth, currentDay, currentHour, currentMinute, powerGeneration, powerConsumption, voltage);

        bool success = httpsGET(data);

        client.stop();
        return success;
    };

    uint16_t getRateLimit()
    {
        uint16_t limit = 0;
        if (httpsGET(F("/service/r2/getstatus.jsp"), true))
        {
            // if (client.find("X-Rate-Limit-Remaining: ")) {
            //   client.parseInt()
            // }
            if (client.find("X-Rate-Limit-Limit: "))
            {
                limit = client.parseInt();
            }
            // if (client.find("X-Rate-Limit-Reset: ")) {
            //   client.parseInt()
            // }
        }
        client.stop();
        return limit;
    }

    uint8_t getStatusInterval()
    {
        uint8_t interval = 0;
        if (httpsGET(F("/service/r2/getsystem.jsp")))
        {
            // Remove Header
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
                // 17 don't care about the rest

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
        client.stop();
        return interval;
    }

    const char* HOST PROGMEM = "pvoutput.org";

    WiFiClientSecure client;
    Ticker updateTimer;
    WiFiUDP ntpUDP;
    NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", 3600, 60000);

    double _powerConsumption = 0;
    double _powerGeneration = 0;
    double _panelVoltage = 0;
    int _updateInterval = 0;
} // namespace PVOutput