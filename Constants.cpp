#include "Constants.h"

const char* CONNECTED PROGMEM = "Connected";
const char* DISCONNECTED PROGMEM = "Disconnected";

const char* HOSTNAME PROGMEM = "rngbridge";

const char* OTA_INDEX PROGMEM
    = R"=====(<!DOCTYPE html><html><head><meta charset=utf-8><title>OTA</title></head><body><div class="upload"><form method="POST" action="/ota" enctype="multipart/form-data"><input type="file" name="data" /><input type="submit" name="upload" value="Upload" title="Upload Files"></form></div></body></html>)=====";
