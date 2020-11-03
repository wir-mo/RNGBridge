#pragma once

#include <NTPClient.h>
#include <Ticker.h>
#include <TimeLib.h>
#include <WiFiClientSecure.h>
#include <WiFiUdp.h>

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
         * @param interval Interval of the update in seconds (how long ago was this data read)
         * @param powerGeneration Current power generation in Watts
         * @param powerConsumption  Current power consumption in Watts
         * @param panelVoltage Curret panel voltage in Volts
         *
         * Should be called after data was read from the chargecontroller
         */
        extern void updateData(const double interval, const double powerGeneration, const double powerConsumption,
            const double panelVoltage);
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
     * Tries to get the status interval from PVOutput and if it is valid syncs the time and starts
     * \ref PVOutput::updateTimer for automatic data upload
     */
    extern void start();

    /**
     * @brief Stops the automatic PVOutput data upload
     *
     * Detaches the \ref PVOutput::updateTimer
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
     * @param powerGeneration The generated power in Watts
     * @param powerConsumption The consumed power in Watts
     * @param voltage The panel voltage in Volts
     * @param dataTime The time to update for
     * @return true If data was sent
     * @return false If data was not sent
     */
    extern bool sendPowerData(
        const int powerGeneration, const int powerConsumption, const double voltage, const time_t dataTime);

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
    extern Ticker updateTimer; //< Timer to send data to PVOutput
    extern Ticker startTimer; //< Timer to rettry start
    extern WiFiUDP ntpUDP; //< UDP for \ref PVOutput::timeClient
    extern NTPClient timeClient; //< NTP client for current time

    extern double _powerConsumption; //< Internal counter for power consumption
    extern double _powerGeneration; //< Internal counter for power generation
    extern double _panelVoltage; //< Internal counter for panel voltage
    extern int _updateInterval; //< Internal interval for PVOutput updates in seconds
} // namespace PVOutput
