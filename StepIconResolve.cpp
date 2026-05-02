#include "StepIconResolve.h"

#include "Sc2StaticCosts.h"

#include <algorithm>
#include <cctype>
#include <vector>

namespace
{
void ToLowerAscii(std::string& s)
{
    for (char& c : s)
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
}

void CollapseAsciiWhitespace(std::string& s)
{
    std::string out;
    out.reserve(s.size());
    bool bLastWasSpace = true;
    for (char c : s)
    {
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n')
        {
            if (!bLastWasSpace)
            {
                out.push_back(' ');
                bLastWasSpace = true;
            }
        }
        else
        {
            out.push_back(c);
            bLastWasSpace = false;
        }
    }
    while (!out.empty() && out.back() == ' ')
        out.pop_back();
    s.swap(out);
}

void TrimSegmentAscii(std::string& s)
{
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front())))
        s.erase(s.begin());
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back())))
        s.pop_back();
}

// Ex.: "Hellion x8", "Marine x2", "Zealot ×3" (UTF-8) -> base para keyword match / icone
void StripTrailingXNBatchCount(std::string& s)
{
    for (;;)
    {
        TrimSegmentAscii(s);
        if (s.empty())
            return;
        size_t iEnd = s.size();
        while (iEnd > 0 && std::isspace(static_cast<unsigned char>(s[iEnd - 1])))
            --iEnd;
        if (iEnd == 0)
        {
            s.clear();
            return;
        }
        size_t iNum = iEnd;
        while (iNum > 0 && std::isdigit(static_cast<unsigned char>(s[iNum - 1])))
            --iNum;
        if (iNum == iEnd)
            return;
        size_t iAfterX = iNum;
        while (iAfterX > 0 && std::isspace(static_cast<unsigned char>(s[iAfterX - 1])))
            --iAfterX;
        if (iAfterX == 0)
            return;
        const unsigned char c = static_cast<unsigned char>(s[iAfterX - 1]);
        if (c == 'x' || c == 'X')
        {
            size_t iCut = iAfterX - 1;
            while (iCut > 0 && std::isspace(static_cast<unsigned char>(s[iCut - 1])))
                --iCut;
            s.erase(iCut, iEnd - iCut);
            continue;
        }
        if (iAfterX >= 2 && static_cast<unsigned char>(s[iAfterX - 2]) == 0xC3 && static_cast<unsigned char>(s[iAfterX - 1]) == 0x97)
        {
            size_t iCut = iAfterX - 2;
            while (iCut > 0 && std::isspace(static_cast<unsigned char>(s[iCut - 1])))
                --iCut;
            s.erase(iCut, iEnd - iCut);
            continue;
        }
        return;
    }
}

const char* TryMatchKeyword(const std::string& sLower, const char* kws[], const char* ids[], size_t n)
{
    for (size_t i = 0; i < n; ++i)
    {
        if (sLower.find(kws[i]) != std::string::npos)
            return ids[i];
    }
    return nullptr;
}

const char* GuessTerranIcon(const std::string& noteLower)
{
    static const char* kws[] = {
        "supply depot", "depot", "depósito", "deposito",
        "barracks tech", "barrack tech",
        "barrack", "quartel",
        "refinery", "refinaria",
        "orbital command", "orbital", "comando orbital",
        "planetary fortress", "pf",
        "missile turret", "sensor tower",
        "auto turret", "auto-turret",
        "point defense drone",
        "factory", "fabrica", "fábrica",
        "starport", "porta estelar", "star port",
        "bunker", "centro de comando", "command center", "comando",
        "engineering bay", "engenharia",
        "armory", "arsenal",
        "armas de infantaria nivel 3", "armas de infantaria nivel 2", "armas de infantaria nivel 1",
        "infantry weapons level 3", "infantry weapons level 2", "infantry weapons level 1",
        "infantry weapons +3", "infantry weapons +2", "infantry weapons +1",
        "infantry weapons 3", "infantry weapons 2", "infantry weapons 1",
        "armadura de infantaria nivel 3", "armadura de infantaria nivel 2", "armadura de infantaria nivel 1",
        "infantry armor level 3", "infantry armor level 2", "infantry armor level 1",
        "infantry armor +3", "infantry armor +2", "infantry armor +1",
        "infantry armor 3", "infantry armor 2", "infantry armor 1",
        "fusion core", "nucleo", "núcleo",
        "ghost academy",
        "ghost", "fantasma",
        "tech lab", "reactor", "laboratorio", "laboratório",
        "siege tank", "tanque",
        "viking",
        "widow mine",
        "cyclone",
        "thor",
        "reaper", "ceifador",
        "mule",
        "scv",
        "stimpack", "stim",
        "concussive shells", "concussive",
        "combat shield", "combat shields",
        "cloaking field",
        "marine", "marauder", "hellbat", "hellion", "medivac", "liberator", "raven", "banshee", "battlecruiser",
    };
    static const char* ids[] = {
        "supply_depot", "supply_depot", "supply_depot", "supply_depot",
        "barracks", "barracks",
        "barracks", "barracks",
        "refinery", "refinery",
        "orbital_command", "orbital_command", "orbital_command",
        "planetary_fortress", "planetary_fortress",
        "missile_turret", "sensor_tower",
        "auto-turret", "auto-turret",
        "point_defense_drone",
        "factory", "factory", "factory",
        "starport", "starport", "starport",
        "bunker", "command_center", "command_center", "command_center",
        "engineering_bay", "engineering_bay",
        "armory", "armory",
        "infantry_weapons_3", "infantry_weapons_2", "infantry_weapons_1",
        "infantry_weapons_3", "infantry_weapons_2", "infantry_weapons_1",
        "infantry_weapons_3", "infantry_weapons_2", "infantry_weapons_1",
        "infantry_weapons_3", "infantry_weapons_2", "infantry_weapons_1",
        "infantry_armor_3", "infantry_armor_2", "infantry_armor_1",
        "infantry_armor_3", "infantry_armor_2", "infantry_armor_1",
        "infantry_armor_3", "infantry_armor_2", "infantry_armor_1",
        "infantry_armor_3", "infantry_armor_2", "infantry_armor_1",
        "fusioncore", "fusioncore", "fusioncore",
        "ghost_academy",
        "ghost", "ghost",
        "techlab", "reactor", "techlab", "techlab",
        "siege_tank", "siege_tank",
        "viking",
        "widow_mine",
        "cyclone",
        "thor",
        "reaper", "reaper",
        "mule",
        "scv",
        "stimpack", "stimpack",
        "concussive_shells", "concussive_shells",
        "combat_shield", "combat_shield",
        "cloaking_field",
        "marine", "marauder", "hellbat", "hellion", "medivac", "liberator", "raven", "banshee", "battlecruiser",
    };
    constexpr size_t n = sizeof(kws) / sizeof(kws[0]);
    const char* p = TryMatchKeyword(noteLower, kws, ids, n);
    return p;
}

const char* GuessProtossIcon(const std::string& noteLower)
{
    static const char* kws[] = {
        "high templar", "dark templar", "mothership core",
        "robotics bay", "robotics facility", "twilight council", "cybernetics core",
        "photon cannon", "templar archive", "templar archives",
        "fleet beacon", "dark shrine",
        "shield battery", "stasis ward", "warp prism",
        "void ray", "star gate", "warp gate",
        "nexus", "nexo", "pylon", "pilao", "pilão", "gateway", "portal",
        "stargate",
        "assimilator",
        "cybernetics", "cibernetica", "forge", "forja", "robotics", "twilight",
        "photon", "canhao", "canhão",
        "templar",
        "zealot", "adept", "stalker", "sentry",
        "oracle", "phoenix",
        "immortal", "observer",
        "archon", "carrier", "colossus", "disruptor", "tempest",
        "mothership", "probe",
        "blink", "charge",
    };
    static const char* ids[] = {
        "high_templar", "dark_templar", "mothership_core",
        "robotics_bay", "robotics_facility", "twilight_council", "cybernetics_core",
        "photon_cannon", "templar_archives", "templar_archives",
        "fleet_beacon", "dark_shrine",
        "shieldbattery", "stasisward", "warp_prism",
        "voidray", "stargate", "warp_gate",
        "nexus", "nexus", "pylon", "pylon", "pylon", "gateway", "gateway", "gateway",
        "stargate",
        "assimilator",
        "cybernetics_core", "cybernetics_core", "forge", "forge", "robotics_facility", "twilight_council",
        "photon_cannon", "photon_cannon", "photon_cannon",
        "templar_archives",
        "zealot", "adept", "stalker", "sentry",
        "oracle", "phoenix",
        "immortal", "observer",
        "archon", "carrier", "colossus", "disruptor", "tempest",
        "mothership", "probe",
        "blink", "charge",
    };
    constexpr size_t n = sizeof(kws) / sizeof(kws[0]);
    return TryMatchKeyword(noteLower, kws, ids, n);
}

const char* GuessZergIcon(const std::string& noteLower)
{
    static const char* kws[] = {
        "greater spire", "grande espira",
        "nydus network", "nydus canal",
        "nydus worm", "nydus",
        "lurker den",
        "hydralisk den", "hydra den",
        "baneling nest", "bane nest",
        "roach warren",
        "infestation pit", "infestation",
        "ultralisk cavern",
        "spawning pool", "poco", "poço", "pool",
        "evolution chamber",
        "spine crawler", "spore crawler",
        "creep tumor",
        "spire",
        "hatchery", "ninhada",
        "lair", "colmeia",
        "hive", "enxame",
        "extractor", "extractora", "refinery", "refinaria", "gas",
        "brood lord", "swarm host",
        "hydralisk", "hidralisco", "hydra",
        "lurker", "espreitador",
        "baneling",
        "roach", "barata",
        "ravager", "devorador",
        "mutalisk", "mutalisco",
        "corruptor",
        "infestor",
        "ultralisk", "ultralisco",
        "viper", "vibora",
        "queen", "rainha",
        "overseer", "supervisor",
        "overlord",
        "zergling", "speedling",
        "drone", "zangao",
        "metabolic boost", "boost metabolico",
        "adrenal glands", "glandulas adrenais",
        "melee weapons level 3", "melee weapons level 2", "melee weapons level 1",
        "missile weapons level 3", "missile weapons level 2", "missile weapons level 1",
        "ground carapace level 3", "ground carapace level 2", "ground carapace level 1",
        "flyer attack level 3", "flyer attack level 2", "flyer attack level 1",
        "flyer armor level 3", "flyer armor level 2", "flyer armor level 1",
        "centrifugal hooks",
        "tunneling claws",
        "grooved spines",
        "muscular augments",
        "adaptive talons",
        "glial reconstitution",
        "seismic spines",
        "chitinous plating",
        "anabolic synthesis",
        "pneumatized carapace",
        "burrow",
    };
    static const char* ids[] = {
        "greater_spire", "greater_spire",
        "nydus_network", "nydus_network",
        "nydus_worm", "nydus_network",
        "lurker_den",
        "hydralisk_den", "hydralisk_den",
        "baneling_nest", "baneling_nest",
        "roach_warren",
        "infestation_pit", "infestation_pit",
        "ultralisk_cavern",
        "spawning_pool", "spawning_pool", "spawning_pool", "spawning_pool",
        "evolution_chamber",
        "spine_crawler", "spore_crawler",
        "creep_tumor",
        "spire",
        "hatchery", "hatchery",
        "lair", "lair",
        "hive", "hive",
        "extractor", "extractor", "extractor", "extractor", "extractor",
        "brood_lord", "swarm_host",
        "hydralisk", "hydralisk", "hydralisk",
        "lurker", "lurker",
        "baneling",
        "roach", "roach",
        "ravager", "ravager",
        "mutalisk", "mutalisk",
        "corruptor",
        "infestor",
        "ultralisk", "ultralisk",
        "viper", "viper",
        "queen", "queen",
        "overseer", "overseer",
        "overlord",
        "zergling", "zergling",
        "drone", "drone",
        "metabolic_boost", "metabolic_boost",
        "adrenal_glands", "adrenal_glands",
        "melee_weapons_3", "melee_weapons_2", "melee_weapons_1",
        "missile_weapons_3", "missile_weapons_2", "missile_weapons_1",
        "ground_carapace_3", "ground_carapace_2", "ground_carapace_1",
        "flyer_attack_3", "flyer_attack_2", "flyer_attack_1",
        "flyer_armor_3", "flyer_armor_2", "flyer_armor_1",
        "centrifugal_hooks",
        "tunneling_claws",
        "grooved_spines",
        "muscular_augments",
        "adaptive_talons",
        "glial_reconstitution",
        "seismic_spines",
        "chitinous_plating",
        "anabolic_synthesis",
        "pneumatized_carapace",
        "burrow",
    };
    constexpr size_t n = sizeof(kws) / sizeof(kws[0]);
    return TryMatchKeyword(noteLower, kws, ids, n);
}

const char* GuessIconForRace(BuildRace eRace, const std::string& nNormalizedLower)
{
    switch (eRace)
    {
    case BuildRace::Terran: return GuessTerranIcon(nNormalizedLower);
    case BuildRace::Protoss: return GuessProtossIcon(nNormalizedLower);
    case BuildRace::Zerg: return GuessZergIcon(nNormalizedLower);
    default: return GuessTerranIcon(nNormalizedLower);
    }
}

std::vector<std::string> ResolveStepIconIdsBody(const BuildStep& step, BuildRace eRace)
{
    if (!step.sIconId.empty())
        return { step.sIconId };
    std::vector<std::string> vOut;
    const std::string& sRaw = step.sNote;
    size_t iStart = 0;
    while (iStart <= sRaw.size())
    {
        const size_t iComma = sRaw.find(',', iStart);
        std::string sPart = (iComma == std::string::npos) ? sRaw.substr(iStart) : sRaw.substr(iStart, iComma - iStart);
        TrimSegmentAscii(sPart);
        if (const size_t iNl = sPart.find_first_of("\r\n"); iNl != std::string::npos)
            sPart.erase(iNl);
        TrimSegmentAscii(sPart);
        if (sPart.empty())
        {
            if (iComma == std::string::npos)
                break;
            iStart = iComma + 1;
            continue;
        }
        StripTrailingParentheticalClauses(sPart);
        TrimSegmentAscii(sPart);
        StripTrailingXNBatchCount(sPart);
        if (sPart.empty())
        {
            if (iComma == std::string::npos)
                break;
            iStart = iComma + 1;
            continue;
        }
        ToLowerAscii(sPart);
        CollapseAsciiWhitespace(sPart);
        if (const char* p = GuessIconForRace(eRace, sPart))
        {
            std::string sId(p);
            if (vOut.empty() || vOut.back() != sId)
                vOut.push_back(std::move(sId));
        }
        if (iComma == std::string::npos)
            break;
        iStart = iComma + 1;
    }
    if (vOut.empty())
        return { "unknown" };
    return vOut;
}
} // namespace

std::vector<std::string> ResolveStepIconIds(const BuildStep& step, BuildRace eRace)
{
    return ResolveStepIconIdsBody(step, eRace);
}
