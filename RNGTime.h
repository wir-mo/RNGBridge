#pragma once

#include <Arduino.h>

class RNGTime
{
public:
    RNGTime(/* args */);

    void loop();

    /// @brief Get the epoch time
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

    String getFormattedTime() const { return getFormattedTime(getEpochTime()); }
    String getFormattedTime(const uint32_t time) const;

    /// @brief Check if the time has been synced
    ///
    /// @return true if time has been synced
    /// @return false if not
    bool isSynced() const;

private:
    enum State
    {
        SYNC_TIME,
        SYNCED_TIME,
    } _state
        = State::SYNC_TIME;

private:
    constexpr static const char* _NTP1 = "pool.ntp.org";
    constexpr static const char* _NTP2 = "time.nist.gov";

    int32_t _offsetS = 0; /// Time offset in seconds
};
