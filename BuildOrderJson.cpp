#include "BuildOrderJson.h"

#include "Sc2StaticCosts.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cctype>

using nlohmann::json;

static BuildRace ParseRaceString(const std::string& s)
{
    std::string t = s;
    std::transform(t.begin(), t.end(), t.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    if (t == "terran" || t == "t")
        return BuildRace::Terran;
    if (t == "protoss" || t == "p")
        return BuildRace::Protoss;
    if (t == "zerg" || t == "z")
        return BuildRace::Zerg;
    return BuildRace::Unknown;
}

static const char* RaceToString(BuildRace eRace)
{
    switch (eRace)
    {
    case BuildRace::Terran: return "Terran";
    case BuildRace::Protoss: return "Protoss";
    case BuildRace::Zerg: return "Zerg";
    default: return "Unknown";
    }
}

bool BuildOrderFromJsonString(const std::string& sJson, BuildOrder& outOrder, std::string& outError)
{
    outError.clear();
    outOrder.Clear();
    try
    {
        const json j = json::parse(sJson);
        if (!j.is_object())
        {
            outError = "JSON root must be an object.";
            return false;
        }
        if (!j.contains("name") || !j["name"].is_string())
        {
            outError = "Missing string field \"name\".";
            return false;
        }
        if (!j.contains("steps") || !j["steps"].is_array())
        {
            outError = "Missing array field \"steps\".";
            return false;
        }
        outOrder.sName = j["name"].get<std::string>();
        if (j.contains("race") && j["race"].is_string())
            outOrder.eRace = ParseRaceString(j["race"].get<std::string>());
        for (const auto& step : j["steps"])
        {
            if (!step.is_object())
                continue;
            BuildStep bs;
            if (step.contains("supply") && step["supply"].is_number_integer())
                bs.iSupply = step["supply"].get<int>();
            if (step.contains("time") && step["time"].is_number())
            {
                bs.fTimeSec = step["time"].get<float>();
                bs.bTimeSpecified = true;
            }
            if (step.contains("note") && step["note"].is_string())
                bs.sNote = step["note"].get<std::string>();
            StripTrailingParentheticalClauses(bs.sNote);
            if (step.contains("notes") && step["notes"].is_string())
                bs.sRowNotes = step["notes"].get<std::string>();
            if (step.contains("icon") && step["icon"].is_string())
                bs.sIconId = step["icon"].get<std::string>();
            if (step.contains("minerals") && step["minerals"].is_number_integer())
                bs.iMinerals = step["minerals"].get<int>();
            if (step.contains("gas") && step["gas"].is_number_integer())
                bs.iGas = step["gas"].get<int>();
            if (step.contains("build_time") && step["build_time"].is_number_integer())
                bs.iBuildTimeSec = step["build_time"].get<int>();
            {
                const int qNote = ParseNoteTrailingBatchQuantity(bs.sNote);
                bs.iQuantity = qNote;
                if (step.contains("quantity") && step["quantity"].is_number_integer())
                {
                    const int qj = (std::max)(1, step["quantity"].get<int>());
                    if (qNote <= 1 && qj > 1)
                        bs.iQuantity = qj;
                }
            }
            outOrder.vSteps.push_back(std::move(bs));
        }
        return true;
    }
    catch (const std::exception& ex)
    {
        outError = ex.what();
        return false;
    }
}

bool BuildOrderToJsonString(const BuildOrder& order, std::string& outJson, std::string& outError)
{
    outError.clear();
    try
    {
        json j;
        j["name"] = order.sName;
        j["race"] = RaceToString(order.eRace);
        json steps = json::array();
        for (const BuildStep& st : order.vSteps)
        {
            json js;
            if (st.iSupply >= 0)
                js["supply"] = st.iSupply;
            if (st.bTimeSpecified)
                js["time"] = st.fTimeSec;
            std::string sNoteOut = st.sNote;
            StripTrailingParentheticalClauses(sNoteOut);
            js["note"] = std::move(sNoteOut);
            if (!st.sRowNotes.empty())
                js["notes"] = st.sRowNotes;
            if (!st.sIconId.empty())
                js["icon"] = st.sIconId;
            if (st.iMinerals >= 0)
                js["minerals"] = st.iMinerals;
            if (st.iGas >= 0)
                js["gas"] = st.iGas;
            if (st.iBuildTimeSec >= 0)
                js["build_time"] = st.iBuildTimeSec;
            if (st.iQuantity > 1)
                js["quantity"] = st.iQuantity;
            steps.push_back(std::move(js));
        }
        j["steps"] = std::move(steps);
        outJson = j.dump(2);
        return true;
    }
    catch (const std::exception& ex)
    {
        outError = ex.what();
        return false;
    }
}
