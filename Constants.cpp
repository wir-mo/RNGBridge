#include "Constants.h"

const char* CONNECTED PROGMEM = "Connected";
const char* DISCONNECTED PROGMEM = "Disconnected";

const char* HOSTNAME PROGMEM = "rngbridge";

char deviceMAC[13];

const uint32_t RENOGY_INTERVAL = 1; /// The interval in s at which the renogy data should be read