#include "Sc2StaticCosts.h"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <string>
#include <vector>

// Dados de custo equivalentes ao catalogo multiplayer (SC2API: UnitTypeData::mineral_cost / vespene_cost).

static void TrimInPlace(std::string& s)
{
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back())))
        s.pop_back();
    size_t i0 = 0;
    while (i0 < s.size() && std::isspace(static_cast<unsigned char>(s[i0])))
        ++i0;
    if (i0 > 0)
        s.erase(0, i0);
}

void StripTrailingParentheticalClauses(std::string& s)
{
    for (;;)
    {
        while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back())))
            s.pop_back();
        if (s.empty() || s.back() != ')')
            return;
        const size_t iClose = s.size() - 1;
        const size_t iOpen = s.rfind('(', iClose);
        if (iOpen == std::string::npos)
            return;
        size_t iCut = iOpen;
        if (iCut > 0 && std::isspace(static_cast<unsigned char>(s[iCut - 1])))
            --iCut;
        s.erase(iCut);
    }
}

static void ToLowerAsciiInPlace(std::string& s)
{
    for (char& c : s)
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
}

static bool ParseSegmentQtyAndName(std::string seg, std::string& outNameNorm, int& outQty)
{
    StripTrailingParentheticalClauses(seg);
    TrimInPlace(seg);
    if (seg.empty())
        return false;
    ToLowerAsciiInPlace(seg);
    outQty = 1;
    const size_t xPos = seg.rfind(" x");
    if (xPos != std::string::npos && xPos + 2 < seg.size())
    {
        std::string numPart = seg.substr(xPos + 2);
        TrimInPlace(numPart);
        if (!numPart.empty())
        {
            int q = 0;
            bool bOk = true;
            for (unsigned char c : numPart)
            {
                if (!std::isdigit(c))
                {
                    bOk = false;
                    break;
                }
                q = q * 10 + static_cast<int>(c - '0');
                if (q > 999)
                {
                    bOk = false;
                    break;
                }
            }
            if (bOk && q > 0)
            {
                outNameNorm = seg.substr(0, xPos);
                TrimInPlace(outNameNorm);
                outQty = q;
                return !outNameNorm.empty();
            }
        }
    }
    size_t iEnd = seg.size();
    while (iEnd > 0U && std::isspace(static_cast<unsigned char>(seg[iEnd - 1])))
        --iEnd;
    if (iEnd > 0U)
    {
        size_t iNumStart = iEnd;
        while (iNumStart > 0U && std::isdigit(static_cast<unsigned char>(seg[iNumStart - 1])))
            --iNumStart;
        if (iNumStart < iEnd)
        {
            int q = 0;
            bool bDigitsOk = true;
            for (size_t k = iNumStart; k < iEnd; ++k)
            {
                const unsigned char d = static_cast<unsigned char>(seg[k]);
                if (!std::isdigit(d))
                {
                    bDigitsOk = false;
                    break;
                }
                q = q * 10 + static_cast<int>(d - '0');
            }
            if (bDigitsOk && q > 0 && q <= 999)
            {
                size_t j = iNumStart;
                while (j > 0U && std::isspace(static_cast<unsigned char>(seg[j - 1])))
                    --j;
                if (j >= 1U && seg[j - 1] == 'x')
                {
                    std::string base = seg.substr(0, j - 1);
                    TrimInPlace(base);
                    if (!base.empty())
                    {
                        outNameNorm = std::move(base);
                        outQty = q;
                        return true;
                    }
                }
                if (j >= 2U && static_cast<unsigned char>(seg[j - 2]) == 0xC3 && static_cast<unsigned char>(seg[j - 1]) == 0x97)
                {
                    std::string base = seg.substr(0, j - 2);
                    TrimInPlace(base);
                    if (!base.empty())
                    {
                        outNameNorm = std::move(base);
                        outQty = q;
                        return true;
                    }
                }
            }
        }
    }
    outNameNorm = std::move(seg);
    return true;
}

int ParseNoteTrailingBatchQuantity(const std::string& sNote)
{
    std::string seg = sNote;
    std::string nameNorm;
    int qty = 1;
    if (!ParseSegmentQtyAndName(seg, nameNorm, qty))
        return 1;
    return (std::max)(1, qty);
}

namespace
{
struct PhraseRow
{
    const char* sPhrase;
    int iMinerals;
    int iGas;
};

static const PhraseRow kTerranPhrases[] = {
    {"infernal pre-igniter", 100, 100},
    {"infernal pre igniter", 100, 100},
    {"enhanced shockwaves", 100, 100},
    {"hyperflight rotors", 150, 150},
    {"advanced ballistics", 150, 150},
    {"concussive shells", 50, 50},
    {"planetary fortress", 150, 150},
    {"barracks tech lab", 200, 25},
    {"barracks reactor", 200, 50},
    {"factory tech lab", 200, 125},
    {"factory reactor", 200, 150},
    {"starport tech lab", 200, 125},
    {"starport reactor", 200, 150},
    {"infantry weapons", 100, 100},
    {"infantry armor", 100, 100},
    {"vehicle weapons", 100, 100},
    {"vehicle armor", 100, 100},
    {"ship weapons", 100, 100},
    {"command center", 400, 0},
    {"orbital command", 150, 0},
    {"ghost academy", 150, 50},
    {"engineering bay", 125, 0},
    {"missile turret", 100, 0},
    {"combat shield", 100, 100},
    {"cloaking field", 100, 100},
    {"moebius reactor", 100, 100},
    {"yamato cannon", 100, 100},
    {"sensor tower", 125, 100},
    {"supply depot", 100, 0},
    {"drilling claws", 75, 75},
    {"fusion core", 150, 150},
    {"siege tank", 150, 125},
    {"battlecruiser", 400, 300},
    {"widow mine", 75, 25},
    {"viking fighter", 150, 75},
    {"liberator", 150, 150},
    {"marauder", 100, 25},
    {"medivac", 100, 100},
    {"hellion", 100, 0},
    {"banshee", 150, 100},
    {"stimpack", 100, 100},
    {"tech lab", 50, 25},
    {"factory", 150, 100},
    {"starport", 150, 100},
    {"barracks", 150, 0},
    {"refinery", 75, 0},
    {"armory", 150, 100},
    {"reactor", 50, 50},
    {"bunker", 100, 0},
    {"thor", 300, 200},
    {"raven", 100, 150},
    {"viking", 150, 75},
    {"ghost", 150, 125},
    {"marine", 50, 0},
    {"reaper", 50, 50},
    {"ebay", 125, 0},
    {"cc", 400, 0},
    {"pf", 150, 150},
    {"bc", 400, 300},
    {"planetary", 150, 150},
    {"orbital", 150, 0},
    {"stim", 100, 100},
    {"tank", 150, 125},
    {"mine", 75, 25},
    {"scv", 50, 0},
};

static const PhraseRow kZergPhrases[] = {
    {"glial reconstitution", 100, 100},
    {"tunneling claws", 100, 100},
    {"grooved spines", 100, 100},
    {"muscular augments", 150, 150},
    {"metabolic boost", 100, 100},
    {"adrenal glands", 200, 200},
    {"chitinous plating", 150, 150},
    {"anabolic synthesis", 150, 150},
    {"pneumatized carapace", 100, 100},
    {"ventral sacs", 200, 200},
    {"burrow movement", 150, 150},
    {"adaptive talons", 150, 150},
    {"seismic spines", 150, 150},
    {"greater spire", 150, 150},
    {"nydus network", 150, 150},
    {"nydus worm", 50, 50},
    {"ultralisk cavern", 150, 200},
    {"infestation pit", 150, 100},
    {"hydralisk den", 100, 100},
    {"lurker den", 150, 150},
    {"baneling nest", 100, 50},
    {"roach warren", 150, 0},
    {"evolution chamber", 75, 0},
    {"spawning pool", 200, 0},
    {"spine crawler", 100, 0},
    {"spore crawler", 75, 0},
    {"swarm host", 200, 100},
    {"brood lord", 150, 150},
    {"broodlord", 150, 150},
    {"hatchery", 300, 0},
    {"lair", 150, 100},
    {"hive", 200, 150},
    {"spire", 200, 200},
    {"infestor", 150, 150},
    {"mutalisk", 100, 100},
    {"hydralisk", 100, 50},
    {"corruptor", 150, 100},
    {"ultralisk", 300, 200},
    {"zergling", 50, 0},
    {"baneling", 25, 25},
    {"ravager", 25, 25},
    {"lurker", 50, 100},
    {"overlord", 100, 0},
    {"overseer", 50, 50},
    {"queen", 150, 0},
    {"drone", 50, 0},
    {"roach", 75, 25},
    {"viper", 100, 200},
    {"extractor", 25, 0},
    {"hatch", 300, 0},
    {"pool", 200, 0},
    {"hydra", 100, 50},
    {"muta", 100, 100},
    {"ultra", 300, 200},
    {"bling", 25, 25},
    {"ling", 50, 0},
    {"nydus", 150, 150},
    {"melee weapons", 100, 100},
    {"missile weapons", 100, 100},
    {"ranged weapons", 100, 100},
    {"melee attack", 100, 100},
    {"flyer weapons", 100, 100},
    {"flyer attack", 100, 100},
    {"flyer armor", 100, 100},
    {"carapace", 100, 100},
    {"ground armor", 100, 100},
};

static const PhraseRow kProtossPhrases[] = {
    {"resonating glaives", 100, 100},
    {"extended thermal lance", 150, 150},
    {"anion pulse crystals", 150, 150},
    {"flux vanes", 100, 100},
    {"gravitic drive", 100, 100},
    {"gravitic boosters", 100, 100},
    {"warp gate", 50, 50},
    {"templar archive", 150, 200},
    {"twilight council", 150, 100},
    {"cybernetics core", 150, 0},
    {"cyber core", 150, 0},
    {"robotics facility", 200, 100},
    {"robotics bay", 150, 150},
    {"fleet beacon", 300, 200},
    {"dark shrine", 150, 150},
    {"shield battery", 100, 0},
    {"photon cannon", 150, 0},
    {"stargate", 150, 150},
    {"gateway", 150, 0},
    {"assimilator", 75, 0},
    {"forge", 150, 0},
    {"pylon", 100, 0},
    {"nexus", 400, 0},
    {"high templar", 50, 150},
    {"dark templar", 125, 125},
    {"warp prism", 200, 0},
    {"void ray", 150, 100},
    {"carrier", 350, 250},
    {"mothership", 300, 300},
    {"tempest", 250, 175},
    {"oracle", 150, 100},
    {"phoenix", 150, 100},
    {"disruptor", 150, 150},
    {"colossus", 300, 200},
    {"observer", 25, 75},
    {"immortal", 275, 100},
    {"sentry", 50, 100},
    {"stalker", 125, 50},
    {"adept", 100, 0},
    {"zealot", 100, 0},
    {"probe", 50, 0},
    {"charge", 100, 100},
    {"blink", 150, 150},
    {"shadow stride", 100, 100},
    {"khaydarin amulet", 150, 150},
    {"ground weapons", 100, 100},
    {"ground armor", 100, 100},
    {"air weapons", 100, 100},
    {"air armor", 100, 100},
    {"shields", 100, 100},
    {"shield", 100, 100},
    {"cannon", 150, 0},
    {"prism", 200, 0},
};

static std::vector<const PhraseRow*> g_vTerranSorted;
static std::vector<const PhraseRow*> g_vZergSorted;
static std::vector<const PhraseRow*> g_vProtossSorted;
static bool g_bTerranSort = false;
static bool g_bZergSort = false;
static bool g_bProtossSort = false;

void SortPhraseTable(const PhraseRow* pRows, size_t nCount, std::vector<const PhraseRow*>& vOut, bool& bDone)
{
    if (bDone)
        return;
    bDone = true;
    vOut.reserve(nCount);
    for (size_t i = 0; i < nCount; ++i)
        vOut.push_back(&pRows[i]);
    std::sort(vOut.begin(), vOut.end(), [](const PhraseRow* a, const PhraseRow* b) {
        const size_t na = std::strlen(a->sPhrase);
        const size_t nb = std::strlen(b->sPhrase);
        if (na != nb)
            return na > nb;
        return std::strcmp(a->sPhrase, b->sPhrase) < 0;
    });
}

bool LookupOneNormalizedName(const std::string& nameNorm, int& outMn, int& outGas, const std::vector<const PhraseRow*>& vRows)
{
    if (nameNorm.empty())
        return false;
    for (const PhraseRow* pRow : vRows)
    {
        if (nameNorm == pRow->sPhrase)
        {
            outMn = pRow->iMinerals;
            outGas = pRow->iGas;
            return true;
        }
    }
    for (const PhraseRow* pRow : vRows)
    {
        if (std::strlen(pRow->sPhrase) <= 2U && nameNorm != pRow->sPhrase)
            continue;
        if (nameNorm.find(pRow->sPhrase) != std::string::npos)
        {
            outMn = pRow->iMinerals;
            outGas = pRow->iGas;
            return true;
        }
    }
    return false;
}

bool ComputeNoteCost(const std::string& sNote, int& outMinerals, int& outGas, const std::vector<const PhraseRow*>& vRows)
{
    outMinerals = 0;
    outGas = 0;
    std::string s = sNote;
    TrimInPlace(s);
    if (s.empty())
        return false;
    bool bAny = false;
    std::string cur;
    auto FlushSeg = [&]()
    {
        if (cur.empty())
            return;
        StripTrailingParentheticalClauses(cur);
        TrimInPlace(cur);
        if (cur.empty())
            return;
        std::string nameNorm;
        int qty = 1;
        if (!ParseSegmentQtyAndName(cur, nameNorm, qty))
            return;
        int m = 0;
        int g = 0;
        if (!LookupOneNormalizedName(nameNorm, m, g, vRows))
            return;
        outMinerals += m * qty;
        outGas += g * qty;
        bAny = true;
    };
    for (char c : s)
    {
        if (c == ',' || c == ';')
        {
            FlushSeg();
            cur.clear();
        }
        else
            cur.push_back(c);
    }
    FlushSeg();
    return bAny;
}
} // namespace

void ApplySc2AutoCostsToBuildSteps(BuildRace eRace, std::vector<BuildStep>& vSteps)
{
    const std::vector<const PhraseRow*>* pRows = nullptr;
    switch (eRace)
    {
    case BuildRace::Terran:
        SortPhraseTable(kTerranPhrases, sizeof(kTerranPhrases) / sizeof(kTerranPhrases[0]), g_vTerranSorted, g_bTerranSort);
        pRows = &g_vTerranSorted;
        break;
    case BuildRace::Zerg:
        SortPhraseTable(kZergPhrases, sizeof(kZergPhrases) / sizeof(kZergPhrases[0]), g_vZergSorted, g_bZergSort);
        pRows = &g_vZergSorted;
        break;
    case BuildRace::Protoss:
        SortPhraseTable(kProtossPhrases, sizeof(kProtossPhrases) / sizeof(kProtossPhrases[0]), g_vProtossSorted, g_bProtossSort);
        pRows = &g_vProtossSorted;
        break;
    default:
        for (BuildStep& st : vSteps)
        {
            st.iMinerals = -1;
            st.iGas = -1;
        }
        return;
    }
    for (BuildStep& st : vSteps)
    {
        int m = 0;
        int g = 0;
        if (ComputeNoteCost(st.sNote, m, g, *pRows))
        {
            st.iMinerals = m;
            st.iGas = g;
        }
        else
        {
            st.iMinerals = -1;
            st.iGas = -1;
        }
    }
}
