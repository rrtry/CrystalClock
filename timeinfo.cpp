#include "timeinfo.h"
#include <iomanip>
#include <sstream>

using namespace std;

float GetElapsedSecondsMinute(Time t) 
{
    return t.second + (t.millisecond / 1000.f);
}

float GetElapsedSecondsHour(Time t) 
{
    return t.minute * 60.f + GetElapsedSecondsMinute(t);
}

float GetElapsedSecondsDay(Time t) 
{
    return t.hour * 3600.f + GetElapsedSecondsHour(t);
}

void GetElapsedSeconds(ElapsedSeconds* s, Time t) 
{
    s->minute = t.second + (t.millisecond / 1000.f);
    s->hour   = t.minute * 60.f + s->minute;
    s->day    = t.hour * 3600.f + s->hour;
}

void GetLocalTime(Time* timeInfo, TimePoint now)
{
    time_t now_time_t = chrono::system_clock::to_time_t(now);
    tm local_time;

#ifdef _WIN32
    localtime_s(&local_time, &now_time_t);
#else
    localtime_r(&local_time, &now_time_t);
#endif

    int year   = local_time.tm_year;
    int month  = local_time.tm_mon;
    int day    = local_time.tm_mday;
    int hour   = local_time.tm_hour;
    int minute = local_time.tm_min;
    int second = local_time.tm_sec;

    auto epoch_time = now.time_since_epoch();
    int millisecond = chrono::duration_cast<chrono::milliseconds>(epoch_time).count() % 1000;

    timeInfo->timePoint   = now;
    timeInfo->year        = year;
    timeInfo->month       = month;
    timeInfo->day         = day;

    timeInfo->hour        = hour;
    timeInfo->minute      = minute;
    timeInfo->second      = second;
    timeInfo->millisecond = millisecond;
}

void GetTimeInfo(Time* timeInfo, TimePoint now)
{
    GetLocalTime(timeInfo, now);
}

void GetTimeInfo(Time* timeInfo) 
{
    GetLocalTime(timeInfo, chrono::system_clock::now());
}

string PutTime(const tm& localTime, const char* timeLocale, const char* format)
{
    ostringstream oss;
    oss.imbue(std::locale(timeLocale));
    oss << put_time(&localTime, format);

    return oss.str();
}

string FormatDate(const Time& time, const char* timeLocale)
{
    tm localTime;
    localTime.tm_year = time.year;
    localTime.tm_mon  = time.month;
    localTime.tm_mday = time.day;
    
    return PutTime(localTime, timeLocale, "%x");
}

string FormatTime(const Time& time, const char* timeLocale)
{
    tm localTime;
    localTime.tm_hour = time.hour;
    localTime.tm_min  = time.minute;
    localTime.tm_sec  = time.second;

    return PutTime(localTime, timeLocale, "%X");
}