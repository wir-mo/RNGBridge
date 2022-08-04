#pragma once

#if (defined(__AVR__))
#include <avr\pgmspace.h>
#else
#include <pgmspace.h>
#endif

#define SIMULATED_DEMO_DATA 1
#define CONST_DEMO_DATA 2
//#define DEMO_MODE SIMULATED_DEMO_DATA // Enable demo mode with dummy data
#define DEBUG_SERIAL Serial1
//#define DEBUG_SERIAL Serial
//#define DEBUG_CONFIG // Uncomment to dump config on startup

#ifdef DEBUG_SERIAL
#define DEBUG(s) DEBUG_SERIAL.print(s)
#define DEBUGLN(s) DEBUG_SERIAL.println(s)
#if defined(__cplusplus) && (__cplusplus > 201703L)
#define DEBUGF(format, ...) DEBUG_SERIAL.printf_P(PSTR(format), __VA_OPT__(, ) __VA_ARGS__)
#else // !(defined(__cplusplus) && (__cplusplus >  201703L))
#define DEBUGF(format, ...) DEBUG_SERIAL.printf_P(PSTR(format), ##__VA_ARGS__)
#endif
#else
#define DEBUG(s)
#define DEBUGLN(s)
#define DEBUGF(format, ...)
#endif

// Note version is made up of
// Major Changes . New Features . Bugfixes (aka major.minor.bug)
constexpr static const char* SOFTWARE_VERSION = "2.7.3";
constexpr static const char* HARDWARE_VERSION = "V2";
constexpr static const char* MODEL = "RNGBridge";

extern const char* CONNECTED PROGMEM;
extern const char* DISCONNECTED PROGMEM;

extern const char* HOSTNAME PROGMEM;

/// MAC adress of ESP8266
// size: 13 chars
extern char deviceMAC[13];

extern const uint32_t RENOGY_INTERVAL; /// The interval in s at which the renogy data should be read

namespace RNGBridge
{
    extern int32_t rssi; /// WiFi signal strength in dBm
} // namespace RNGBridge
