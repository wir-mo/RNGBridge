#pragma once

#include <avr/pgmspace.h>


#define HAVE_GUI

// const static String CONNECTED = "Connected";
// const static String DISCONNECTED = "Disconnected";

extern const char* CONNECTED PROGMEM;
extern const char* DISCONNECTED PROGMEM;

extern const char* hostname PROGMEM;