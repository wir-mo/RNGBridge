#pragma once

#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>

#include "Constants.h"
#include "GUI.h"
#include "RSTime.h"

/// @brief Class for checking for software updates and in future for updating the software programatically
class OTA
{
public:
    /// @brief Construct a new OTA object
    ///
    /// @param versionTag The tag of the current running software version
    /// @param gui Reference to GUI object
    /// @param time Reference to RSTime object
    OTA(const char* versionTag, GUI& gui, RSTime& time);

    /// @brief Get the version tag of the latest software version
    ///
    /// @return Version tag
    String getNewSoftwareVersion();

    /// @brief Schedule checking for a new update
    void checkForUpdate()
    {
        if (_state == State::IDLE)
        {
            _state = State::CHECK_FOR_VERSION;
        }
    }

    /// @brief Update internal state
    ///
    /// Call this once in a while
    void loop();

private:
    /// @brief Internal state
    enum State
    {
        IDLE, /// Nothing to be done
        CHECK_FOR_VERSION, /// Check for new software version
    } _state
        = State::IDLE;

private:
    constexpr static const char* GHOTA_HOST = "api.github.com";
    constexpr static const uint16_t GHOTA_PORT = 443;
    constexpr static const char* GHOTA_CONTENT_TYPE = "application/octet-stream";

    constexpr static const char* GHOTA_USER = "enwi";
    constexpr static const char* GHOTA_REPO = "RNGBridge";
    constexpr static const char* GHOTA_FILE = "RSBridge.bin";
    constexpr static const bool GHOTA_ACCEPT_PRERELEASE = false;

    const char* _versionTag;

    GUI& _gui;
    RSTime& _time;
};
