#pragma once

#include <Arduino.h>

/// @brief Class for handling time stuff like NTP
class RNGTime
{
public:
    /// @brief Construct a new RNGTime object
    ///
    /// Also sets up NTP servers
    RNGTime();

    /// @brief Update internal state
    void loop();

    /// @brief Get the epoch time with offset
    ///
    /// @return time in seconds since Jan. 1, 1970
    uint32_t getEpochTime() const;

    /// @brief Get the current time as tm (with corrected year and month)
    ///
    /// @return struct tm
    struct tm getTmTime() const;

    /// @brief Set the time offset in seconds
    ///
    /// @param seconds time offset
    void setTimeOffset(const int32_t seconds);

    /// @brief Get a time string of the current time
    ///
    /// @return Time string like hh:mm:ss
    String getFormattedTime() const { return getFormattedTime(getEpochTime()); }
    /// @brief Get a time string of the given time
    ///
    /// @param time Epoch time
    /// @return Time string like hh:mm:ss
    String getFormattedTime(const uint32_t time) const;

    /// @brief Check if the time has been synced
    ///
    /// @return true if time has been synced
    /// @return false if not
    bool isSynced() const;

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
