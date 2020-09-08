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
        extern void updateData(const uint8_t interval, const int powerGeneration, const int powerConsumption);

        extern time_t getNTPTime();
    } // namespace Callback

    extern void setup();

    extern void start();

    extern void stop();

    extern bool httpsGET(const String& url, const bool rateLimit = false);

    extern bool httpsGET(const char* url, const bool rateLimit = false);

    extern bool httpsGET(
        WiFiClientSecure& client, const char* url, const char* apiKey, const char* sysID, const bool rateLimit = false);

    /**
     * @brief s
     *
     * @param powerGeneration Generated power in watts
     * @param powerConsumption Consumed power in watts
     * @return true On success
     * @return false On error
     */
    extern bool sendPowerData(const int powerGeneration, const int powerConsumption, const time_t dataTime);

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

    extern bool forceTimeSync();

    constexpr const char* host = "pvoutput.org";
    constexpr const char* PVApiKey = "08124bfa21591165ca948b67ddd87152c4e8eabb";
    constexpr const char* PVSysID = "72583";

    extern WiFiClientSecure client;
    extern Ticker updateTimer;
    extern WiFiUDP ntpUDP;
    extern NTPClient timeClient;

    extern double _powerConsumption;
    extern double _powerGeneration;
    extern int _updateInterval;
} // namespace PVOutput
