#pragma once

#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>

#include "Constants.h"
#include "GUI.h"
#include "RNGTime.h"

class OTA
{
public:
    OTA(const char* versionTag, GUI& gui, RNGTime& time);

    String getNewSoftwareVersion();

    void checkForUpdate()
    {
        if (_state == State::IDLE)
        {
            _state = State::CHECK_FOR_VERSION;
        }
    }

    void loop();

private:
    enum State
    {
        IDLE,
        CHECK_FOR_VERSION,
    } _state
        = State::IDLE;

private:
    constexpr static const char* GHOTA_HOST = "api.github.com";
    constexpr static const uint16_t GHOTA_PORT = 443;
    constexpr static const char* GHOTA_CONTENT_TYPE = "application/octet-stream";

    constexpr static const char* GHOTA_USER = "enwi";
    constexpr static const char* GHOTA_REPO = "RNGBridgeDoc";
    constexpr static const char* GHOTA_FILE = "RNGBridge.ino.bin";
    constexpr static const bool GHOTA_ACCEPT_PRERELEASE = false;

    const char* _versionTag;

    GUI& _gui;
    RNGTime& _time;
};
