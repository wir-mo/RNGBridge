#pragma once

#include <NTPClient.h>
#include <TimeLib.h>
#include <WiFiClientSecure.h>
#include <WiFiUdp.h>

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

namespace PVOutput
{
    namespace Callback
    {
        /**
         * @brief Send the geneated power, consumed power and voltage data to PVOutput
         *
         * Should be called at a specific interval given by \ref PVOutput::getStatusInterval
         */
        extern void sendData();
        /**
         * @brief Update the current solar data
         *
         * @param energyGeneration Current energy generation in Wh
         * @param powerGeneration Current power generation in W
         * @param energyConsumption Current energy consumption in Wh
         * @param powerConsumption Current power consumption in W
         * @param temperature A temperature in °C
         * @param voltage A voltage in Volts
         *
         * Should be called after data was read from the chargecontroller
         */
        extern void updateData(const int16_t energyGeneration, const double powerGeneration,
            const int16_t energyConsumption, const double powerConsumption, const double temperature,
            const double voltage);
    } // namespace Callback

    /**
     * @brief Setup clients and other stuff needed
     *
     * Sets up WiFiClientSecure \ref PVOutput::client and the NTPClient \ref PVOutput::timeClient.
     */
    extern void setup();

    /**
     * @brief Tries to start automatic PVOutput data upload
     *
     * Tries to get the status interval from PVOutput and if it is valid syncs the time and set
     * \ref PVOutput::started true
     */
    extern void start();

    /**
     * @brief Stops the automatic PVOutput data upload
     *
     * Sets \ref PVOutput::started false
     */
    extern void stop();

    /**
     * @brief Updates the internal state
     *
     * Checks the settings whether to start or stop the client by either calling \ref PVOutput::start
     * or \ref PVOutput::stop
     */
    extern void update();

    /**
     * @brief Forces to sync the NTP \ref PVOutput::timeClient if WiFi is connected
     *
     * @return true If NTP was synced
     * @return false If NTP could not be synced
     */
    extern bool syncTime();

    /**
     * @brief Make an HTTP GET request to the given url
     *
     * @param url URL to make request to
     * @param rateLimit Have rate limit in header (used for \ref PVOutput::getRateLimit)
     * @return true If \ref PVOutput::client could connect and sent data
     * @return false If \ref PVOutput::client could not connect
     */
    extern bool httpsGET(const String& url, const bool rateLimit = false);

    /**
     * @brief Make an HTTP GET request to the given url
     *
     * @param url URL to make request to
     * @param rateLimit Have rate limit in header (used for \ref PVOutput::getRateLimit)
     * @return true If \ref PVOutput::client could connect and sent data
     * @return false If \ref PVOutput::client could not connect
     */
    extern bool httpsGET(const char* url, const bool rateLimit = false);

    /**
     * @brief Make an HTTP GET request to the given url
     *
     * @param client Client to use
     * @param url URL to make request to
     * @param apiKey PVOutput API Key
     * @param sysID PVOutput system ID
     * @param rateLimit Have rate limit in header (used for \ref PVOutput::getRateLimit)
     * @return true If \ref PVOutput::client could connect and sent data
     * @return false If \ref PVOutput::client could not connect
     */
    extern bool httpsGET(WiFiClientSecure& client, const char* url, const char* apiKey, const uint32_t sysID,
        const bool rateLimit = false);

    /**
     * @brief Send the generated power, consumed power and panel voltage data to PVOutput
     *
     * @param energyGeneration The energy generation in Wh
     * @param powerGeneration The power generation in W
     * @param energyConsumption The energy consumption in Wh
     * @param powerConsumption The power consumption in W
     * @param temperature A temperature in °C
     * @param voltage A voltage in Volts
     * @param dataTime The time to update for
     * @return true If data was sent
     * @return false If data was not sent
     */
    extern bool sendPowerData(const int16_t energyGeneration, const double powerGeneration,
        const int16_t energyConsumption, const double powerConsumption, const double temperature, const double voltage,
        const time_t dataTime);

    /**
     * @brief Get rate limit
     *
     * Aka how many requests can be made to PVOutput every hour
     *
     * @return uint16_t The rate limit
     */
    extern uint16_t getRateLimit();

    /**
     * @brief Get status interval at which to update the status
     *
     * @return uint8_t The interval in minutes
     */
    extern uint8_t getStatusInterval();

    extern const char* HOST PROGMEM; //< Host to make rrequests to aka pvoutput.org

    extern WiFiClientSecure client; //< Client to make requests with
    extern WiFiUDP ntpUDP; //< UDP for \ref PVOutput::timeClient
    extern NTPClient timeClient; //< NTP client for current time

    extern int16_t _energyGeneration; /// Internal energy generation value
    extern int16_t _energyConsumption; /// Internal energy consumption value
    extern ApproxRollingAverage<double, 60 / 2> _powerGeneration; /// Internal average for power generation in W
    extern ApproxRollingAverage<double, 60 / 2> _powerConsumption; /// Internal average for power consumption in W
    extern ApproxRollingAverage<double, 60 / 2> _voltage; /// Internal average for voltage
    extern ApproxRollingAverage<double, 60 / 2> _temperature; /// Internal average for temperature
    extern int _updateInterval; //< Internal interval for PVOutput updates in seconds

    extern bool _initial; /// Is this the initial data?
    extern bool started; //< Did we start
} // namespace PVOutput
