#pragma once

#include <functional>

#include <WiFiClientSecure.h>

#include "Config.h"
#include "Constants.h"
#include "Observerable.h"
#include "RNGTime.h"
#include "Renogy.h"


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
    PVOutput(const PVOutputConfig& config, RNGTime& time) : _config(config), _time(time)
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
    void sendData();

    ///@brief Update the current solar data
    ///
    ///@param data Renogy data
    ///
    /// Should be called after data was read from the chargecontroller
    void updateData(const Renogy::Data& data);

    ///@brief Tries to start automatic PVOutput data upload
    ///
    /// Tries to get the status interval from PVOutput and if it is valid syncs the time and sets _started true
    void start();

    ///@brief Updates the internal state
    ///
    /// Uploads data to PVOutput and resets counters
    /// Should be called once every second.
    void loop();

private:
    ///@brief Make an HTTP GET request to the given url
    ///
    ///@param url URL to make request to
    ///@param rateLimit Have rate limit in header (used for \ref PVOutput::getRateLimit)
    ///@return true If \ref PVOutput::client could connect and sent data
    ///@return false If \ref PVOutput::client could not connect
    bool httpsGET(const String& url, const bool rateLimit = false);

    ///@brief Make an HTTP GET request to the given url
    ///
    ///@param url URL to make request to
    ///@param rateLimit Have rate limit in header (used for \ref PVOutput::getRateLimit)
    ///@return true If \ref PVOutput::client could connect and sent data
    ///@return false If \ref PVOutput::client could not connect
    bool httpsGET(const char* url, const bool rateLimit = false);

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
        const bool rateLimit = false);

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
        const uint16_t powerConsumption, const double temperature, const double voltage, const tm& tm);

    ///@brief Get rate limit
    ///
    /// Aka how many requests can be made to PVOutput every hour
    ///
    ///@return uint16_t The rate limit
    uint16_t getRateLimit();

    ///@brief Get status interval at which to update the status
    ///
    ///@return uint8_t The interval in minutes
    uint8_t getStatusInterval();

private:
    static const char* HOST PROGMEM; /// Host to make rrequests to aka pvoutput.org

    const PVOutputConfig& _config;
    RNGTime& _time;

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
