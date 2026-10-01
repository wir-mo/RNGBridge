#pragma once

#if (defined(__AVR__))
#include <avr\pgmspace.h>
#else
#include <pgmspace.h>
#endif

#include <HardwareSerial.h>

#define SIMULATED_DEMO_DATA 1
#define CONST_DEMO_DATA 2
// #define DEMO_MODE SIMULATED_DEMO_DATA // Enable demo mode with dummy data
#define RS_DEBUG_SERIAL Serial1
// #define RS_DEBUG_SERIAL Serial
// #define DEBUG_CONFIG // Uncomment to dump config on startup

#ifdef RS_DEBUG_SERIAL
#define RS_DEBUG(s) RS_DEBUG_SERIAL.print(s)
#define RS_DEBUGLN(s) RS_DEBUG_SERIAL.println(s)
#if defined(__cplusplus) && (__cplusplus > 201703L)
#define RS_DEBUGF(format, ...) RS_DEBUG_SERIAL.printf_P(PSTR(format), __VA_OPT__(, ) __VA_ARGS__)
#else // !(defined(__cplusplus) && (__cplusplus >  201703L))
#define RS_DEBUGF(format, ...) RS_DEBUG_SERIAL.printf_P(PSTR(format), ##__VA_ARGS__)
#endif
#else
#define RS_DEBUG(s)
#define RS_DEBUGLN(s)
#define RS_DEBUGF(format, ...)
#endif

// Note version is made up of
// Major Changes . New Features . Bugfixes (aka major.minor.bug)
constexpr static const char* SOFTWARE_VERSION = "2.12.0";
constexpr static const char* MODEL = "RSBridge";

extern const char* CONNECTED PROGMEM;
extern const char* DISCONNECTED PROGMEM;

extern const char* HOSTNAME PROGMEM;

/// MAC adress of ESP8266
// size: 13 chars
extern char deviceMAC[13];

constexpr static const uint32_t RENOGY_INTERVAL = 2; /// The interval in s at which the renogy data should be read

namespace RSBridge
{
    extern int32_t rssi; /// WiFi signal strength in dBm

    //     namespace Logger
    //     {
    //         template <typename... Args>
    //         void debug(const char* c, Args... args)
    //         {
    // #ifdef RS_DEBUG_SERIAL
    //             RS_DEBUG_SERIAL.printf_P(PSTR(c), args...);
    // #endif
    //         }

    //         void debug(const char* t)
    //         {
    // #ifdef RS_DEBUG_SERIAL
    //             RS_DEBUG_SERIAL.print(t);
    // #endif
    //         }
    //     } // namespace Logger

} // namespace RSBridge
