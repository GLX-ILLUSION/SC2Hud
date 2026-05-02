#pragma once

// Tempo da partida (relogio de parede): Start now, pausa/retoma, sincronizar segundos.
class MatchClock
{
public:
    void Reset();
    void StartNow();
    void Pause();
    void Resume();

    bool IsRunning() const { return bRunning; }

    double GetElapsedSeconds() const;

    void SetElapsedSeconds(double dSeconds);

private:
    bool bRunning = false;

    long long iFreq = 0;
    long long iStart = 0;
    double dPausedAccum = 0.0;
    long long iPauseMark = 0;
};
