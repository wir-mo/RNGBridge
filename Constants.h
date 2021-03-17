#pragma once

#if (defined(__AVR__))
#include <avr\pgmspace.h>
#else
#include <pgmspace.h>
#endif

// Enable OTA support (only useful if Flash is >1MB)
// #define OTA

// const static String CONNECTED = "Connected";
// const static String DISCONNECTED = "Disconnected";

extern const char* CONNECTED PROGMEM;
extern const char* DISCONNECTED PROGMEM;

extern const char* HOSTNAME PROGMEM;
extern const char* OTA_INDEX PROGMEM;