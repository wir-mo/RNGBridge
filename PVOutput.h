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
        extern void sendData();
        extern void updateData(const uint8_t interval, const double powerGeneration, const double powerConsumption,
            const double panelVoltage);
    } // namespace Callback

    extern void setup();

    extern void start();

    extern void stop();

    extern void update();

    extern bool syncTime();

    extern bool httpsGET(const String& url, const bool rateLimit = false);

    extern bool httpsGET(const char* url, const bool rateLimit = false);

    extern bool httpsGET(WiFiClientSecure& client, const char* url, const char* apiKey, const uint32_t sysID,
        const bool rateLimit = false);

    /**
     * @brief s
     *
     * @param powerGeneration Generated power in watts
     * @param powerConsumption Consumed power in watts
     * @return true On success
     * @return false On error
     */
    extern bool sendPowerData(
        const int powerGeneration, const int powerConsumption, const double voltage, const time_t dataTime);

    /**
     * @brief Get rate limit
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

    extern const char* HOST PROGMEM;

    extern WiFiClientSecure client;
    extern Ticker updateTimer;
    extern WiFiUDP ntpUDP;
    extern NTPClient timeClient;

    extern double _powerConsumption;
    extern double _powerGeneration;
    extern double _panelVoltage;
    extern int _updateInterval;
} // namespace PVOutput
