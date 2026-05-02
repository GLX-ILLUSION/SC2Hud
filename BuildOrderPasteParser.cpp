#include "BuildOrderPasteParser.h"

#include "Sc2StaticCosts.h"

#include <cctype>
#include <cstring>
#include <sstream>
#include <string>
#include <vector>

static void TitleToFileBase(const std::string& title, char* cBuf, size_t nBuf)
{
    if (nBuf == 0)
        return;
    cBuf[0] = 0;
    size_t iW = 0;
    for (unsigned char c : title)
    {
        if (iW + 1 >= nBuf)
            break;
        if (std::isalnum(c))
            cBuf[iW++] = static_cast<char>(c);
        else if (c == ' ' || c == '-' || c == '_' || c == '.' || c == '(' || c == ')')
        {
            if (iW > 0 && cBuf[iW - 1] != '_')
                cBuf[iW++] = '_';
        }
    }
    while (iW > 0 && cBuf[iW - 1] == '_')
        --iW;
    cBuf[iW] = 0;
    if (iW == 0)
        (void)std::snprintf(cBuf, nBuf, "imported_build");
}

namespace
{
void TrimInPlace(std::string& s)
{
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back())))
        s.pop_back();
    size_t iStart = 0;
    while (iStart < s.size() && std::isspace(static_cast<unsigned char>(s[iStart])))
        ++iStart;
    if (iStart > 0)
        s.erase(0, iStart);
}

static bool ParsePositiveInt(const std::string& s, size_t i0, size_t i1, int& outV)
{
    if (i0 >= i1 || i1 > s.size())
        return false;
    int v = 0;
    for (size_t i = i0; i < i1; ++i)
    {
        const unsigned char c = static_cast<unsigned char>(s[i]);
        if (!std::isdigit(c))
            return false;
        v = v * 10 + static_cast<int>(c - '0');
        if (v > 100000000)
            return false;
    }
    outV = v;
    return true;
}

// Dois segmentos = minutos:segundos (ex.: 0:17, 30:00, 185:20). Tres = hora:min:seg (ex.: 1:30:05).
bool ParseMmSsToken(const std::string& tok, float& outSec)
{
    const size_t c0 = tok.find(':');
    if (c0 == std::string::npos || c0 == 0)
        return false;
    const size_t c1 = tok.find(':', c0 + 1);
    int iTotalSec = 0;
    if (c1 == std::string::npos)
    {
        int iMin = 0;
        int iSec = 0;
        if (!ParsePositiveInt(tok, 0, c0, iMin))
            return false;
        if (!ParsePositiveInt(tok, c0 + 1, tok.size(), iSec))
            return false;
        if (iSec > 59)
            return false;
        if (iMin > 10000)
            return false;
        iTotalSec = iMin * 60 + iSec;
    }
    else
    {
        if (tok.find(':', c1 + 1) != std::string::npos)
            return false;
        int iH = 0;
        int iMin = 0;
        int iSec = 0;
        if (!ParsePositiveInt(tok, 0, c0, iH))
            return false;
        if (!ParsePositiveInt(tok, c0 + 1, c1, iMin))
            return false;
        if (!ParsePositiveInt(tok, c1 + 1, tok.size(), iSec))
            return false;
        if (iMin > 59 || iSec > 59)
            return false;
        if (iH > 999)
            return false;
        iTotalSec = iH * 3600 + iMin * 60 + iSec;
    }
    if (iTotalSec > 86400 * 6)
        return false;
    outSec = static_cast<float>(iTotalSec);
    return true;
}

// Apos supply e tempo: "Titulo<TAB>dicas" ou "Titulo  dicas" (2+ espacos). So titulo fica em note e texto extra em row_notes (JSON "notes").
static void SplitTitleAndExtraNotes(std::string sRest, std::string& outNote, std::string& outRowNotes)
{
    outNote.clear();
    outRowNotes.clear();
    TrimInPlace(sRest);
    if (sRest.empty())
        return;

    const size_t iTab = sRest.find('\t');
    if (iTab != std::string::npos)
    {
        outNote = sRest.substr(0, iTab);
        outRowNotes = sRest.substr(iTab + 1);
        TrimInPlace(outNote);
        TrimInPlace(outRowNotes);
        return;
    }

    size_t iSplit = std::string::npos;
    for (size_t i = 1; i + 1 < sRest.size(); ++i)
    {
        if (std::isspace(static_cast<unsigned char>(sRest[i])))
        {
            size_t j = i;
            while (j < sRest.size() && std::isspace(static_cast<unsigned char>(sRest[j])))
                ++j;
            if (j - i >= 2)
            {
                iSplit = i;
                break;
            }
        }
    }
    if (iSplit != std::string::npos)
    {
        outNote = sRest.substr(0, iSplit);
        outRowNotes = sRest.substr(iSplit);
        TrimInPlace(outNote);
        TrimInPlace(outRowNotes);
    }
    else
        outNote = std::move(sRest);
}

bool TryParseStepLine(const std::string& line, BuildStep& outStep)
{
    size_t iPos = 0;
    while (iPos < line.size() && std::isspace(static_cast<unsigned char>(line[iPos])))
        ++iPos;
    if (iPos >= line.size() || !std::isdigit(static_cast<unsigned char>(line[iPos])))
        return false;
    const size_t iSupplyStart = iPos;
    while (iPos < line.size() && std::isdigit(static_cast<unsigned char>(line[iPos])))
        ++iPos;
    if (iSupplyStart == iPos)
        return false;
    int iSupply = 0;
    if (!ParsePositiveInt(line, iSupplyStart, iPos, iSupply))
        return false;
    if (iSupply < 1 || iSupply > 220)
        return false;
    while (iPos < line.size() && std::isspace(static_cast<unsigned char>(line[iPos])))
        ++iPos;
    const size_t iTimeStart = iPos;
    while (iPos < line.size() && (std::isdigit(static_cast<unsigned char>(line[iPos])) || line[iPos] == ':'))
        ++iPos;
    if (iTimeStart == iPos)
        return false;
    const std::string sTimeTok = line.substr(iTimeStart, iPos - iTimeStart);
    float fSec = 0.f;
    if (!ParseMmSsToken(sTimeTok, fSec))
        return false;
    while (iPos < line.size() && std::isspace(static_cast<unsigned char>(line[iPos])))
        ++iPos;
    std::string sRest = line.substr(iPos);
    TrimInPlace(sRest);
    if (sRest.empty())
        return false;
    std::string sNote;
    std::string sRowNotes;
    SplitTitleAndExtraNotes(std::move(sRest), sNote, sRowNotes);
    if (sNote.empty())
        return false;
    StripTrailingParentheticalClauses(sNote);
    TrimInPlace(sNote);
    if (sNote.empty())
        return false;
    outStep.iSupply = iSupply;
    outStep.fTimeSec = fSec;
    outStep.bTimeSpecified = true;
    outStep.sNote = std::move(sNote);
    outStep.sRowNotes = std::move(sRowNotes);
    outStep.iQuantity = ParseNoteTrailingBatchQuantity(outStep.sNote);
    return true;
}

BuildRace InferRaceFromTitle(const std::string& sTitle)
{
    std::string t;
    t.reserve(sTitle.size());
    for (unsigned char c : sTitle)
        t.push_back(static_cast<char>(std::tolower(c)));
    const auto Has = [&t](const char* p) { return t.find(p) != std::string::npos; };
    if (Has("zvz") || Has("zvt") || Has("zvp"))
        return BuildRace::Zerg;
    if (Has("pvp") || Has("pvt") || Has("pvz"))
        return BuildRace::Protoss;
    if (Has("tvt") || Has("tvp") || Has("tvz"))
        return BuildRace::Terran;
    if (Has("zerg"))
        return BuildRace::Zerg;
    if (Has("protoss"))
        return BuildRace::Protoss;
    if (Has("terran"))
        return BuildRace::Terran;
    return BuildRace::Unknown;
}
} // namespace

bool ParseWebsiteBuildOrderPaste(const std::string& sPaste, BuildOrder& outOrder, std::string& outError)
{
    outError.clear();
    outOrder.Clear();

    std::string sRemaining;
    if (sPaste.size() >= 3 && static_cast<unsigned char>(sPaste[0]) == 0xEF && static_cast<unsigned char>(sPaste[1]) == 0xBB && static_cast<unsigned char>(sPaste[2]) == 0xBF)
        sRemaining = sPaste.substr(3);
    else
        sRemaining = sPaste;

    std::vector<std::string> vTitleLines;
    std::vector<BuildStep> vSteps;
    std::istringstream iss(sRemaining);
    std::string line;
    while (std::getline(iss, line))
    {
        if (!line.empty() && line.back() == '\r')
            line.pop_back();
        TrimInPlace(line);
        if (line.empty())
            continue;
        BuildStep step{};
        if (TryParseStepLine(line, step))
        {
            vSteps.push_back(std::move(step));
            continue;
        }
        if (vSteps.empty())
            vTitleLines.push_back(line);
    }

    if (vSteps.empty())
    {
        outError = "Nenhuma linha no formato \"supply  m:ss  acao\" encontrada. Opcional: tab ou 2+ espacos e depois dicas longas (coluna extra). Tempos m:ss ou h:mm:ss.";
        return false;
    }

    std::string sName;
    for (size_t i = 0; i < vTitleLines.size(); ++i)
    {
        if (i > 0)
            sName += ' ';
        sName += vTitleLines[i];
    }
    if (sName.empty())
        sName = "Build importada";

    outOrder.sName = std::move(sName);
    outOrder.eRace = InferRaceFromTitle(outOrder.sName);
    outOrder.vSteps = std::move(vSteps);
    return true;
}

void BuildOrderPasteSuggestedFileBase(const std::string& sBuildName, char* cBuf, size_t nBuf)
{
    TitleToFileBase(sBuildName, cBuf, nBuf);
}
