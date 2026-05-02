#pragma once

#include <string>
#include <vector>

enum class BuildRace : int
{
    Unknown = 0,
    Terran,
    Protoss,
    Zerg,
};

struct BuildStep
{
    int iSupply = -1;
    float fTimeSec = 0.f;
    bool bTimeSpecified = false;
    std::string sNote;
    std::string sIconId;
    int iQuantity = 1;
    int iMinerals = -1;
    int iGas = -1;
    int iBuildTimeSec = -1;
    std::string sRowNotes;
};

struct BuildOrder
{
    std::string sName;
    BuildRace eRace = BuildRace::Unknown;
    std::vector<BuildStep> vSteps;

    void Clear()
    {
        sName.clear();
        eRace = BuildRace::Unknown;
        vSteps.clear();
    }
};
