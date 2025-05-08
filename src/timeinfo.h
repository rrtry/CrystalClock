#ifndef TIMEINFO_H
#define TIMEINFO_H

#include <chrono>
#include <string>

using namespace std;
typedef chrono::system_clock::time_point TimePoint;

typedef struct 
{
    TimePoint timePoint;
    int year;
    int month;
    int day;

    int hour;
    int minute;
    int second;
    int millisecond;
} Time;

typedef struct 
{
    float minute;
    float hour;
    float day;
} ElapsedSeconds;

float GetElapsedSecondsMinute(Time t);
float GetElapsedSecondsHour(Time t);
float GetElapsedSecondsDay(Time t);

void GetElapsedSeconds(ElapsedSeconds* s, Time t);
void GetTimeInfo(Time* timeInfo);
void GetTimeInfo(Time* timeInfo, TimePoint timePoint);

string FormatDate(const Time& time, const char* timeLocale);
string FormatTime(const Time& time, const char* timeLocale);

#endif
