#include "MatchClock.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

void MatchClock::Reset()
{
    bRunning = false;
    dPausedAccum = 0.0;
    iStart = 0;
    iPauseMark = 0;
    if (iFreq == 0)
    {
        LARGE_INTEGER f;
        QueryPerformanceFrequency(&f);
        iFreq = f.QuadPart;
    }
}

void MatchClock::StartNow()
{
    if (iFreq == 0)
    {
        LARGE_INTEGER f;
        QueryPerformanceFrequency(&f);
        iFreq = f.QuadPart;
    }
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    iStart = now.QuadPart;
    dPausedAccum = 0.0;
    bRunning = true;
}

void MatchClock::Pause()
{
    if (!bRunning)
        return;
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    iPauseMark = now.QuadPart;
    bRunning = false;
}

void MatchClock::Resume()
{
    if (bRunning)
        return;
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    const double dPauseSec = static_cast<double>(now.QuadPart - iPauseMark) / static_cast<double>(iFreq);
    dPausedAccum += dPauseSec;
    bRunning = true;
}

double MatchClock::GetElapsedSeconds() const
{
    if (!bRunning)
        return dPausedAccum;
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    const double dSec = static_cast<double>(now.QuadPart - iStart) / static_cast<double>(iFreq);
    return dSec + dPausedAccum;
}

void MatchClock::SetElapsedSeconds(double dSeconds)
{
    if (iFreq == 0)
    {
        LARGE_INTEGER f;
        QueryPerformanceFrequency(&f);
        iFreq = f.QuadPart;
    }
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    iStart = now.QuadPart;
    dPausedAccum = dSeconds;
    bRunning = true;
}
