#pragma once

#include <functional>

#include <WiFiClientSecure.h>

#include "Config.h"
#include "Constants.h"
#include "Observerable.h"
#include "RSTime.h"

#if defined(ESP32)
#include <Update.h>
#else
#include <ESP8266WiFi.h>
#include <Updater.h>
#include <include/WiFiState.h>
#endif

/// @brief Class for approximating a rolling average
///
/// @tparam T Value type to average
/// @tparam N Number of values to average (approximation)
template <typename T, unsigned N>
class ApproxRollingAverage
{
public:
    /// @brief Construct a new Approx Rolling Average object
    ///
    /// @param initial Initial value
    ApproxRollingAverage(const T& initial = 0) : average(initial) { }

    /// @brief Conversion operator
    ///
    /// @return Average value
    operator T() const { return average; };

    /// @brief Asignment operator, set average to specific value
    ///
    /// @param value New average value
    /// @return ApproxRollingAverage&
    ApproxRollingAverage& operator=(const T& value)
    {
        average = value;
        return *this;
    };

    /// @brief + operator
    ///
    /// @param value Value to add to average
    /// @return ApproxRollingAverage&
    ApproxRollingAverage& operator+(const T& value) { return this += value; }

    /// @brief += operator
    ///
    /// @param value Value to add to average
    /// @return ApproxRollingAverage&
    ApproxRollingAverage& operator+=(const T& value)
    {
        average -= average / N;
        average += value / N;
        return *this;
    }

private:
    T average; /// Average value
};

class PVOutput : public Observerable<String>
{
public:
    struct Data
    {
        int16_t energyGenerated; /// Generated energy in Wh
        int16_t energyConsumed; /// Consumed energy in Wh
        float powerGeneration; /// Generated power in W
        float powerConsumption; /// Consumed power in W
        float temperature; /// Temperature in °C
        float voltage; /// Voltage in V
    };

public:
    PVOutput(const PVOutputConfig& config, RSTime& time) : _config(config), _time(time)
    {
        // We need to reduce the buffer sizes or we get issues with HEAP
        client.setBufferSizes(4096, 512);
        // Don't want to use Cert Store or Fingerprint cause they need to be updated
        client.setInsecure();

        // Set time offset, convert hours to seconds
        _time.setTimeOffset(_config.timeOffset * 3600);
    };

    PVOutput(PVOutput&&) = delete;

    ///@brief Send the geneated power, consumed power and voltage data to PVOutput
    ///
    /// Should be called at a specific interval given by \ref PVOutput::getStatusInterval
    void sendData()
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
            RS_DEBUGF("[PVO] %s", temp);
            notify(String(temp));
        }
        else
        {
            RS_DEBUGLN(F("[PVO] Could not send power data"));
            notify(F("Could not send power data"));
        }
    }

    ///@brief Update the current solar data
    ///
    ///@param data Renogy data
    ///
    /// Should be called after data was read from the chargecontroller
    void updateData(const Data& data)
    {
        if (_initial)
        {
            _initial = false;
            _powerGeneration = data.powerGeneration;
            _powerConsumption = data.powerConsumption;
            _temperature = data.temperature;
            _voltage = data.voltage;
            return;
        }

        _powerGeneration += data.powerGeneration;
        _powerConsumption += data.powerConsumption;
        _energyGeneration = data.energyGenerated;
        _energyConsumption = data.energyConsumed;
        _temperature += data.temperature;
        _voltage += data.voltage;

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

    ///@brief Tries to start automatic PVOutput data upload
    ///
    /// Tries to get the status interval from PVOutput and if it is valid syncs the time and sets _started true
    void start()
    {
        RS_DEBUGLN(F("[PVO] Starting"));
        notify(F("Starting"));
        // Try to get the status interval which can't be 0
        const uint8_t interval = getStatusInterval();
        if (interval > 0)
        {
            _started = true;

            // Convert minutes to seconds
            _updateInterval = interval * 60;

            // Set status running
            RS_DEBUGLN(F("[PVO] Running"));
            notify(F("Running"));
        }
        else
        {
            _started = false;

            // Set status error
            RS_DEBUGLN(F("[PVO] Could not get update interval, retrying"));
            notify(F("Could not get update interval, retrying"));
        }
    }

    ///@brief Updates the internal state
    ///
    /// Uploads data to PVOutput and resets counters
    /// Should be called once every second.
    void loop()
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

private:
    ///@brief Make an HTTP GET request to the given url
    ///
    ///@param url URL to make request to
    ///@param rateLimit Have rate limit in header (used for \ref PVOutput::getRateLimit)
    ///@return true If \ref PVOutput::client could connect and sent data
    ///@return false If \ref PVOutput::client could not connect
    bool httpsGET(const String& url, const bool rateLimit = false)
    {
        // Delegate
        return httpsGET(url.c_str(), rateLimit);
    }

    ///@brief Make an HTTP GET request to the given url
    ///
    ///@param url URL to make request to
    ///@param rateLimit Have rate limit in header (used for \ref PVOutput::getRateLimit)
    ///@return true If \ref PVOutput::client could connect and sent data
    ///@return false If \ref PVOutput::client could not connect
    bool httpsGET(const char* url, const bool rateLimit = false)
    {
        // Delegate
        return httpsGET(client, url, _config.apiKey.c_str(), _config.systemId, rateLimit);
    }

    ///@brief Make an HTTP GET request to the given url
    ///
    ///@param client Client to use
    ///@param url URL to make request to
    ///@param apiKey PVOutput API Key
    ///@param sysID PVOutput system ID
    ///@param rateLimit Have rate limit in header (used for \ref PVOutput::getRateLimit)
    ///@return true If \ref PVOutput::client could connect and sent data
    ///@return false If \ref PVOutput::client could not connect
    bool httpsGET(WiFiClientSecure& client, const char* url, const char* apiKey, const uint32_t sysID,
        const bool rateLimit = false)
    {
        // RS_DEBUGF("[PVO] GET %s, k: %s, i: %d\n", url, apiKey, sysID);
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
    }

    ///@brief Send the generated energy, consumed energy and panel voltage data to PVOutput
    ///
    ///@param energyGeneration The generated energy in Wh
    ///@param powerGeneration The generated power in W
    ///@param energyConsumption The consumed energy in Wh
    ///@param powerConsumption The consumed power in W
    ///@param voltage A voltage in Volts
    ///@param temperature A temperature in °C
    ///@param tm The time to update for
    ///@return true If data was sent
    ///@return false If data was not sent
    bool sendPowerData(const int16_t energyGeneration, const uint16_t powerGeneration, const int16_t energyConsumption,
        const uint16_t powerConsumption, const double temperature, const double voltage, const tm& tm)
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
    }

    ///@brief Get rate limit
    ///
    /// Aka how many requests can be made to PVOutput every hour
    ///
    ///@return uint16_t The rate limit
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

    ///@brief Get status interval at which to update the status
    ///
    ///@return uint8_t The interval in minutes
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

private:
    constexpr static const char* HOST PROGMEM = "pvoutput.org"; /// Host to make requests to aka pvoutput.org

    const PVOutputConfig& _config;
    RSTime& _time;

    WiFiClientSecure client; /// Client to make requests with

    int16_t _energyGeneration; /// Energy generation in Wh
    int16_t _energyConsumption; /// Energy consumption in Wh
    ApproxRollingAverage<double, 60 / RENOGY_INTERVAL> _powerGeneration; /// Internal average for power generation in W
    ApproxRollingAverage<double, 60 / RENOGY_INTERVAL>
        _powerConsumption; /// Internal average for power consumption in W
    ApproxRollingAverage<double, 60 / RENOGY_INTERVAL> _voltage; /// Internal average for voltage
    ApproxRollingAverage<double, 60 / RENOGY_INTERVAL> _temperature; /// Internal average for temperature
    int _updateInterval = 0.0; /// Internal interval for PVOutput updates in seconds

    bool _started = false; /// Did we start

    uint16_t _secondsPassed = 0; /// amount of seconds passed

    bool _initial = true; /// Did we just start?
}; // class PVOutput
