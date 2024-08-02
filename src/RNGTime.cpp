#include "RNGTime.h"

#include <sntp.h>
#include <time.h>

#include "Constants.h"

RNGTime::RNGTime()
{
    configTime(0, 0, _NTP1, _NTP2); // UTC
}

void RNGTime::loop()
{
    // RNG_DEBUGLN(getFormattedTime(sntp_get_current_timestamp()));
    // RNG_DEBUGLN(getFormattedTime());

    if (_state == State::SYNC_TIME)
    {
        RNG_DEBUGLN("[RNGTime] Synching");
        if (sntp_get_current_timestamp() >= 8 * 3600 * 2)
        {
            _state = State::SYNCED_TIME;
            RNG_DEBUGLN("[RNGTime] Synched");
        }
    }
}

uint32_t RNGTime::getEpochTime() const
{
    return sntp_get_current_timestamp() + _offsetS;
}

struct tm RNGTime::getTmTime() const
{
    time_t epoch = getEpochTime();
    struct tm time;
    gmtime_r(&epoch, &time);
    time.tm_mon += 1;
    time.tm_year += 1900;
    return time;
}

void RNGTime::setTimeOffset(const int32_t offset)
{
    _offsetS = offset;
}

String RNGTime::getFormattedTime(const uint32_t time) const
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

bool RNGTime::isSynced() const
{
    return _state == State::SYNCED_TIME;
}
