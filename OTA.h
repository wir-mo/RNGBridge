#pragma once

#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include <time.h>

#include "Constants.h"

class OTA
{
public:
    OTA(const char* versionTag);

    String getNewSoftwareVersion();

private:
    void updateTime();

private:
    constexpr static const char* GHOTA_NTP1 = "pool.ntp.org";
    constexpr static const char* GHOTA_NTP2 = "time.nist.gov";
    constexpr static const char* GHOTA_HOST = "api.github.com";
    constexpr static const uint16_t GHOTA_PORT = 443;
    constexpr static const char* GHOTA_CONTENT_TYPE = "application/octet-stream";

    constexpr static const char* GHOTA_USER = "enwi";
    constexpr static const char* GHOTA_REPO = "RNGBridgeDoc";
    constexpr static const char* GHOTA_FILE = "RNGBridge.ino.bin";
    constexpr static const bool GHOTA_ACCEPT_PRERELEASE = false;

    const char* _versionTag;
};
