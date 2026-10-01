#pragma once

#include <Arduino.h>
#include <sntp.h>
#include <time.h>

#include "Constants.h"

/// @brief Class for handling time stuff like NTP
class RSTime
{
public:
    /// @brief Construct a new RSTime object
    ///
    /// Also sets up NTP servers
    RSTime()
    {
        configTime(0, 0, _NTP1, _NTP2); // UTC
    }

    /// @brief Update internal state
    void loop()
    {
        // RS_DEBUGLN(getFormattedTime(sntp_get_current_timestamp()));
        // RS_DEBUGLN(getFormattedTime());

        if (_state == State::SYNC_TIME)
        {
            RS_DEBUGLN("[RSTime] Synching");
            if (sntp_get_current_timestamp() >= 8 * 3600 * 2)
            {
                _state = State::SYNCED_TIME;
                RS_DEBUGLN("[RSTime] Synched");
            }
        }
    }

    /// @brief Get the epoch time with offset
    ///
    /// @return time in seconds since Jan. 1, 1970
    uint32_t getEpochTime() const { return sntp_get_current_timestamp() + _offsetS; }

    /// @brief Get the current time as tm (with corrected year and month)
    ///
    /// @return struct tm
    struct tm getTmTime() const
    {
        time_t epoch = getEpochTime();
        struct tm time;
        gmtime_r(&epoch, &time);
        time.tm_mon += 1;
        time.tm_year += 1900;
        return time;
    }

    /// @brief Set the time offset in seconds
    ///
    /// @param seconds time offset
    void setTimeOffset(const int32_t seconds) { _offsetS = seconds; }

    /// @brief Get a time string of the current time
    ///
    /// @return Time string like hh:mm:ss
    String getFormattedTime() const { return getFormattedTime(getEpochTime()); }
    /// @brief Get a time string of the given time
    ///
    /// @param time Epoch time
    /// @return Time string like hh:mm:ss
    String getFormattedTime(const uint32_t time) const
    {
        String str = "";
        str.reserve(8);
        const uint32_t hours = (time % 86400L) / 3600;
        if (hours < 10)
        {
            str += "0";
        }
        str += hours;
        str += ":";

        const uint32_t minutes = (time % 3600) / 60;
        if (minutes < 10)
        {
            str += "0";
        }
        str += minutes;
        str += ":";

        const uint32_t seconds = time % 60;
        if (seconds < 10)
        {
            str += "0";
        }
        str += seconds;

        return str;
    }

    /// @brief Check if the time has been synced
    ///
    /// @return true if time has been synced
    /// @return false if not
    bool isSynced() const { return _state == State::SYNCED_TIME; }

private:
    /// @brief Internal state
    enum State
    {
        SYNC_TIME, /// Time is being synced
        SYNCED_TIME, /// Time has been synced
    } _state
        = State::SYNC_TIME;

private:
    constexpr static const char* _NTP1 = "pool.ntp.org"; /// First NTP pool
    constexpr static const char* _NTP2 = "time.nist.gov"; /// Second NTP pool

    int32_t _offsetS = 0; /// Time offset in seconds
};
