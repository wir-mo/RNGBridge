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
#define RNG_DEBUG_SERIAL Serial1
// #define RNG_DEBUG_SERIAL Serial
// #define DEBUG_CONFIG // Uncomment to dump config on startup

#ifdef RNG_DEBUG_SERIAL
#define RNG_DEBUG(s) RNG_DEBUG_SERIAL.print(s)
#define RNG_DEBUGLN(s) RNG_DEBUG_SERIAL.println(s)
#if defined(__cplusplus) && (__cplusplus > 201703L)
#define RNG_DEBUGF(format, ...) RNG_DEBUG_SERIAL.printf_P(PSTR(format), __VA_OPT__(, ) __VA_ARGS__)
#else // !(defined(__cplusplus) && (__cplusplus >  201703L))
#define RNG_DEBUGF(format, ...) RNG_DEBUG_SERIAL.printf_P(PSTR(format), ##__VA_ARGS__)
#endif
#else
#define RNG_DEBUG(s)
#define RNG_DEBUGLN(s)
#define RNG_DEBUGF(format, ...)
#endif

// Note version is made up of
// Major Changes . New Features . Bugfixes (aka major.minor.bug)
constexpr static const char* SOFTWARE_VERSION = "2.12.0";
constexpr static const char* HARDWARE_VERSION = "V1";
constexpr static const char* MODEL = "EPEBridge";

extern const char* CONNECTED PROGMEM;
extern const char* DISCONNECTED PROGMEM;

extern const char* HOSTNAME PROGMEM;

/// MAC adress of ESP8266
// size: 13 chars
extern char deviceMAC[13];

constexpr static const uint32_t RENOGY_INTERVAL = 2; /// The interval in s at which the renogy data should be read

namespace RNGBridge
{
    extern int32_t rssi; /// WiFi signal strength in dBm

    //     namespace Logger
    //     {
    //         template <typename... Args>
    //         void debug(const char* c, Args... args)
    //         {
    // #ifdef RNG_DEBUG_SERIAL
    //             RNG_DEBUG_SERIAL.printf_P(PSTR(c), args...);
    // #endif
    //         }

    //         void debug(const char* t)
    //         {
    // #ifdef RNG_DEBUG_SERIAL
    //             RNG_DEBUG_SERIAL.print(t);
    // #endif
    //         }
    //     } // namespace Logger

} // namespace RNGBridge
