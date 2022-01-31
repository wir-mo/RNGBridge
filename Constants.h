#pragma once

#if (defined(__AVR__))
#include <avr\pgmspace.h>
#else
#include <pgmspace.h>
#endif

#define DEMO_MODE // Enable demo mode with dummy data
#define DEBUG_SERIAL Serial

#ifdef DEBUG_SERIAL
#define DEBUG(s) DEBUG_SERIAL.print(s)
#define DEBUGLN(s) DEBUG_SERIAL.println(s)
#if defined(__cplusplus) && (__cplusplus > 201703L)
#define DEBUGF(format, ...) DEBUG_SERIAL.printf(format, __VA_OPT__(, ) __VA_ARGS__)
#else // !(defined(__cplusplus) && (__cplusplus >  201703L))
#define DEBUGF(format, ...) DEBUG_SERIAL.printf(format, ##__VA_ARGS__)
#endif
#else
#define DEBUG(s)
#define DEBUGLN(s)
#define DEBUGF(format, ...)
#endif

// const static String CONNECTED = "Connected";
// const static String DISCONNECTED = "Disconnected";

extern const char* CONNECTED PROGMEM;
extern const char* DISCONNECTED PROGMEM;

extern const char* HOSTNAME PROGMEM;

/// MAC adress of ESP8266
// size: 13 chars
extern char deviceMAC[13];