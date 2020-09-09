#include "PVOutput.h"

// TODO improve and cleanup imports

namespace PVOutput
{
    namespace Callback
    {
        void sendData()
        {
            // Serial.println("Free heap: " + String(ESP.getFreeHeap()));
            // TODO make selectable if either panel or system voltage should be sent
            sendPowerData(_powerGeneration, _powerConsumption, 0.0, timeClient.getEpochTime());
            _powerGeneration = 0.0;
            _powerConsumption = 0.0;
        }

        void updateData(const uint8_t interval, const int powerGeneration, const int powerConsumption)
        {
            if (_updateInterval > 0)
            {
                _powerGeneration += ((double)interval / _updateInterval) * powerGeneration;
                _powerConsumption += ((double)interval / _updateInterval) * powerConsumption;
            }
        }
    } // namespace Callback

    void setup()
    {
        client.setInsecure();

        // Setup time client and force time sync
        timeClient.begin();
        bool synced = false;
        while (!synced)
        {
            timeClient.forceUpdate();
            synced = year(timeClient.getEpochTime()) > 2019;
            delay(500);
        }
        // Serial.println("PVOutput Setup done");
    }

    void start()
    {
        // Serial.println("PVOutput starting");
        const uint8_t interval = getStatusInterval();
        if (interval > 0)
        {
            // set status success
            // Send data every interval minutes
            _updateInterval = interval * 60;
            // Serial.println("PVOutput started timer at " + String(_updateInterval) + "s interval");
            updateTimer.attach_scheduled(_updateInterval, PVOutput::Callback::sendData);
        }
        else
        {
            // Set status error
            // Serial.println("PVOutput did not get interval");
        }
    }

    void stop() {}

    bool httpsGET(const String& url, const bool rateLimit) { return httpsGET(url.c_str(), rateLimit); };

    bool httpsGET(const char* url, const bool rateLimit)
    {
        return httpsGET(client, url, PVApiKey, PVSysID, rateLimit);
    };

    bool httpsGET(
        WiFiClientSecure& client, const char* url, const char* apiKey, const char* sysID, const bool rateLimit)
    {
        const bool connected = client.connect(host, 443);
        if (connected)
        {
            client.print(F("GET "));
            client.print(url);
            client.println(F(" HTTP/1.1"));

            client.print(F("Host: "));
            client.println(host);

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
        // USE adjustTime(hours * 60 * 60) for that;
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
        // Serial.println(data);
        // if (success)
        // {
        //     Serial.println("Success");
        // }
        // else
        // {
        //     Serial.println("Failed");
        // }
        client.flush();
        return success;
    };

    uint16_t getRateLimit()
    {
        uint16_t limit = 0;
        if (httpsGET(F("/service/r2/getstatus.jsp"), true))
        {
            // if (client.find("X-Rate-Limit-Remaining: ")) {
            //   Serial.print("remaining: ");
            //   Serial.println(client.parseInt());
            // }
            if (client.find("X-Rate-Limit-Limit: "))
            {
                limit = client.parseInt();
            }
            // if (client.find("X-Rate-Limit-Reset: ")) {
            //   Serial.print("reset: ");
            //   Serial.println(client.parseInt());
            // }
        }
        client.flush();
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
        client.flush();
        return interval;
    }

    WiFiClientSecure client;
    Ticker updateTimer;
    WiFiUDP ntpUDP;
    NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", 3600, 60000);

    double _powerConsumption = 0;
    double _powerGeneration = 0;
    int _updateInterval = 0;
} // namespace PVOutput