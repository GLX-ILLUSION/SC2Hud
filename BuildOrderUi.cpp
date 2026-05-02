#include "BuildOrderUi.h"

#include "BuildOrderJson.h"
#include "BuildOrderPasteParser.h"
#include "BuildOrderPlayback.h"
#include "IconTextureCache.h"
#include "StepIconResolve.h"
#include "Sc2StaticCosts.h"
#include "UiLanguage.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#include <shellapi.h>

namespace
{
constexpr const char kDefaultTerranPreset[] = "presets/pig_terran_b2gm_tvt.json";

void OpenSpawningToolWebsite()
{
    ShellExecuteW(nullptr, L"open", L"https://lotv.spawningtool.com/", nullptr, nullptr, SW_SHOWNORMAL);
}

void HelpMarker(const char* desc)
{
    ImGui::TextDisabled("(?)");
    if (ImGui::BeginItemTooltip())
    {
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 34.0f);
        ImGui::TextUnformatted(desc);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

void PushHudButtonStylePrimary()
{
    ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(12, 45, 82, 255));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(0, 105, 160, 255));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(0, 75, 118, 255));
}

void PushHudButtonStyleAccent()
{
    ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(0, 95, 150, 255));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(0, 170, 220, 255));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(0, 120, 175, 255));
}

void PopHudButtonStyle()
{
    ImGui::PopStyleColor(3);
}

void PushConfigPanelSpacing()
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16.f, 14.f));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(12.f, 10.f));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10.f, 7.f));
}

void PopConfigPanelSpacing()
{
    ImGui::PopStyleVar(3);
}

// Paineis e HUD: alpha 255 para nao misturar com o fundo magenta (chroma key) do back buffer.
constexpr ImU32 kNeonCard       = IM_COL32(12, 22, 44, 255);
constexpr ImU32 kNeonDeep       = IM_COL32(6, 12, 26, 255);
constexpr ImU32 kHudBg          = IM_COL32(8, 14, 28, 255);
constexpr ImU32 kHudBorder      = IM_COL32(0, 145, 200, 255);
constexpr ImU32 kHudAccent      = IM_COL32(0, 200, 255, 255);
constexpr ImU32 kHudRowActive   = IM_COL32(14, 40, 72, 255);
constexpr ImU32 kHudRowStripe   = IM_COL32(10, 20, 38, 255);
constexpr ImU32 kHudSep         = IM_COL32(22, 72, 108, 255);
constexpr ImVec4 kHudInfoText   = ImVec4(0.35f, 0.82f, 1.f, 1.f);

void DrawSupplyCell(int iSupply, float fSizeMul = 1.36f)
{
    if (iSupply < 0)
    {
        ImGui::TextDisabled("—");
        return;
    }
    char b[16];
    std::snprintf(b, sizeof(b), "%d", iSupply);
    ImFont* font = ImGui::GetFont();
    const float fs = ImGui::GetFontSize() * fSizeMul;
    const ImVec2 p = ImGui::GetCursorScreenPos();
    ImGui::GetWindowDrawList()->AddText(font, fs, p, kHudAccent, b);
    const ImVec2 te = font->CalcTextSizeA(fs, FLT_MAX, -1.f, b, nullptr, nullptr);
    ImGui::Dummy(ImVec2(te.x, te.y + 2.f));
}

void DrawCostCell(const BuildStep& st, IconTextureCache* pIcons, float fScale = 1.f)
{
    if (st.iMinerals < 0 && st.iGas < 0 && st.iBuildTimeSec < 0)
    {
        ImGui::TextDisabled("—");
        return;
    }
    bool bFirst = true;
    char buf[80];
    const float fResIcon = 12.f * fScale;
    const ImTextureID unkBase = (pIcons != nullptr) ? pIcons->GetUnknownTexture() : ImTextureID{};
    if (st.iMinerals >= 0)
    {
        if (!bFirst)
            ImGui::SameLine(0.f, 8.f * fScale);
        bFirst = false;
        if (pIcons != nullptr)
        {
            ImTextureID mTex = pIcons->GetIconTexture(BuildRace::Unknown, "minerals");
            const ImVec4 tintM = (mTex == unkBase) ? ImVec4(0.5f, 0.55f, 0.72f, 1.f) : ImVec4(1.f, 1.f, 1.f, 1.f);
            ImGui::ImageWithBg(ImTextureRef(mTex), ImVec2(fResIcon, fResIcon), ImVec2(0.f, 0.f), ImVec2(1.f, 1.f), ImVec4(0.f, 0.f, 0.f, 0.f), tintM);
            ImGui::SameLine(0.f, 4.f * fScale);
            ImGui::AlignTextToFramePadding();
            ImGui::TextColored(ImVec4(0.50f, 0.82f, 1.f, 1.f), "%d", st.iMinerals);
        }
        else
        {
            ImGui::TextColored(ImVec4(0.50f, 0.82f, 1.f, 1.f), "%d", st.iMinerals);
            ImGui::SameLine(0, 2);
            ImGui::TextColored(ImVec4(0.50f, 0.82f, 1.f, 1.f), "M");
        }
    }
    if (st.iGas >= 0)
    {
        if (!bFirst)
            ImGui::SameLine(0.f, 8.f * fScale);
        bFirst = false;
        if (pIcons != nullptr)
        {
            ImTextureID gTex = pIcons->GetIconTexture(BuildRace::Unknown, "gas");
            const ImVec4 tintG = (gTex == unkBase) ? ImVec4(0.5f, 0.55f, 0.72f, 1.f) : ImVec4(1.f, 1.f, 1.f, 1.f);
            ImGui::ImageWithBg(ImTextureRef(gTex), ImVec2(fResIcon, fResIcon), ImVec2(0.f, 0.f), ImVec2(1.f, 1.f), ImVec4(0.f, 0.f, 0.f, 0.f), tintG);
            ImGui::SameLine(0.f, 4.f * fScale);
            ImGui::AlignTextToFramePadding();
            ImGui::TextColored(ImVec4(0.35f, 0.92f, 0.48f, 1.f), "%d", st.iGas);
        }
        else
        {
            ImGui::TextColored(ImVec4(0.35f, 0.92f, 0.48f, 1.f), "%d", st.iGas);
            ImGui::SameLine(0, 2);
            ImGui::TextColored(ImVec4(0.35f, 0.92f, 0.48f, 1.f), "G");
        }
    }
    if (st.iBuildTimeSec >= 0)
    {
        if (!bFirst)
            ImGui::SameLine(0.f, 8.f * fScale);
        std::snprintf(buf, sizeof(buf), "%ds", st.iBuildTimeSec);
        ImGui::TextColored(ImVec4(0.88f, 0.88f, 0.94f, 1.f), "%s", buf);
    }
}

// Uma linha no painel compacto; trunca com "..." se nao couber em fMaxW.
void TextHudHistLineClipped(const char* sNote, float fMaxW)
{
    if (!sNote || !sNote[0] || fMaxW < 10.f)
        return;
    const ImVec2 szFull = ImGui::CalcTextSize(sNote);
    if (szFull.x <= fMaxW)
    {
        ImGui::TextUnformatted(sNote);
        return;
    }
    const char* kEll = "...";
    const float fEllW = ImGui::CalcTextSize(kEll).x;
    const size_t nLen = std::strlen(sNote);
    size_t n = nLen;
    char buf[384];
    while (n > 0U)
    {
        const size_t nCopy = (std::min)(n, sizeof(buf) - 4U);
        std::memcpy(buf, sNote, nCopy);
        buf[nCopy] = '\0';
        if (ImGui::CalcTextSize(buf).x + fEllW <= fMaxW)
        {
            ImGui::Text("%s%s", buf, kEll);
            return;
        }
        --n;
    }
    ImGui::TextUnformatted(kEll);
}

void DrawNotesCell(const std::string& sRowNotes)
{
    if (sRowNotes.empty())
        ImGui::TextDisabled("—");
    else
        ImGui::TextWrapped("%s", sRowNotes.c_str());
}

void DrawFollowAlongCardGlow(ImDrawList* dl, const ImVec2& rmin, const ImVec2& rmax, float rounding, float fPulse01)
{
    const int aCore = static_cast<int>(210.f + 45.f * fPulse01);
    const ImU32 core = IM_COL32(0, 220, 255, aCore);
    for (int k = 4; k >= 1; --k)
    {
        const float pad = static_cast<float>(k) * 1.65f;
        const int a = 14 + k * 9;
        dl->AddRect(ImVec2(rmin.x - pad, rmin.y - pad), ImVec2(rmax.x + pad, rmax.y + pad), IM_COL32(0, 160, 220, a), rounding + pad * 0.22f, 0, 1.0f);
    }
    dl->AddRect(rmin, rmax, core, rounding, 0, 2.0f);
}
} // namespace

void BuildOrderHud::FormatMmSs(float fTimeSec, char* cBuf, size_t nBuf)
{
    const int iT = (std::max)(0, static_cast<int>(fTimeSec + 0.5f));
    const int iMin = iT / 60;
    const int iSec = iT % 60;
    std::snprintf(cBuf, nBuf, "%d:%02d", iMin, iSec);
}

BuildOrderHud::BuildOrderHud(BuildOrderStore& store, MatchClock& matchClock)
    : m_store(store)
    , m_clock(matchClock)
{
    m_loaded.Clear();
    m_editor.Clear();
    std::string err;
    if (m_store.LoadByRelativePath(kDefaultTerranPreset, m_loaded, err))
    {
        m_editor = m_loaded;
        std::snprintf(cSaveFileBase, sizeof(cSaveFileBase), "%s", "pig_terran_b2gm_tvt");
        const std::vector<BuildFileInfo>& v = m_store.GetKnownBuilds();
        for (int i = 0; i < static_cast<int>(v.size()); ++i)
        {
            if (v[static_cast<size_t>(i)].sRelativePath == kDefaultTerranPreset)
            {
                iSelectedList = i;
                break;
            }
        }
    }
}

const char* BuildOrderHud::RaceToLabel(BuildRace eRace)
{
    switch (eRace)
    {
    case BuildRace::Terran: return UiTr(Tr::RaceTerran);
    case BuildRace::Protoss: return UiTr(Tr::RaceProtoss);
    case BuildRace::Zerg: return UiTr(Tr::RaceZerg);
    default: return UiTr(Tr::RaceUnknown);
    }
}

void BuildOrderHud::ClampHudColumnsVisible()
{
    if (!bHudColPop && !bHudColTime && !bHudColAction && !bHudColCost && !bHudColNotes)
        bHudColAction = true;
}

int BuildOrderHud::CountHudColumns() const
{
    int n = 0;
    if (bHudColPop)
        ++n;
    if (bHudColTime)
        ++n;
    if (bHudColAction)
        ++n;
    if (bHudColCost)
        ++n;
    if (bHudColNotes)
        ++n;
    return n;
}

void BuildOrderHud::RenderHudColumnToggles()
{
    ImGui::Dummy(ImVec2(0.f, 4.f));
    ImGui::SeparatorText(UiTr(Tr::ColVisibleTitle));
    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(135, 185, 215, 255));
    ImGui::TextWrapped("%s", UiTr(Tr::ColVisibleHint));
    ImGui::PopStyleColor();
    ImGui::Dummy(ImVec2(0.f, 6.f));
    ImGui::Checkbox(UiTr(Tr::ColPop), &bHudColPop);
    ImGui::SameLine();
    ImGui::Checkbox(UiTr(Tr::ColTime), &bHudColTime);
    ImGui::SameLine();
    ImGui::Checkbox(UiTr(Tr::ColAction), &bHudColAction);
    ImGui::Checkbox(UiTr(Tr::ColCost), &bHudColCost);
    ImGui::SameLine();
    ImGui::Checkbox(UiTr(Tr::ColNotes), &bHudColNotes);
    ClampHudColumnsVisible();
    ImGui::SameLine();
    HelpMarker(UiTr(Tr::HelpColMarker));
}

void BuildOrderHud::SetupAndDrawBuildTable(BuildRace eRace, const std::vector<BuildStep>& vSteps, int iCurrentStep, float fTableHeight, float fIconSz, bool bCfgPreview, bool bScrollCurrentIntoView)
{
    ClampHudColumnsVisible();
    const int nCols = CountHudColumns();
    if (nCols < 1)
        return;

    const ImGuiTableFlags tableFlags = ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_ScrollY
        | (bCfgPreview ? ImGuiTableFlags_SizingStretchProp : ImGuiTableFlags_SizingFixedFit);

    const char* tableId = bCfgPreview ? "steps_cfg" : "step_overlay";
    if (!ImGui::BeginTable(tableId, nCols, tableFlags, ImVec2(0.f, fTableHeight)))
        return;

    const float fPopW = bCfgPreview ? 50.f : 40.f;
    const float fTimeW = bCfgPreview ? 56.f : 44.f;
    const float fCostW = bCfgPreview ? 100.f : 82.f;
    const float fNotesW = bCfgPreview ? 88.f : 70.f;

    if (bHudColPop)
        ImGui::TableSetupColumn(UiTr(Tr::ColPop), ImGuiTableColumnFlags_WidthFixed, fPopW);
    if (bHudColTime)
        ImGui::TableSetupColumn(UiTr(Tr::ColTime), ImGuiTableColumnFlags_WidthFixed, fTimeW);
    if (bHudColAction)
        ImGui::TableSetupColumn(UiTr(Tr::ColAction), ImGuiTableColumnFlags_WidthStretch);
    if (bHudColCost)
        ImGui::TableSetupColumn(UiTr(Tr::ColCost), ImGuiTableColumnFlags_WidthFixed, fCostW);
    if (bHudColNotes)
        ImGui::TableSetupColumn(UiTr(Tr::ColNotes), ImGuiTableColumnFlags_WidthFixed, fNotesW);

    ImGui::TableHeadersRow();

    const int iStepCount = static_cast<int>(vSteps.size());
    for (int i = 0; i < iStepCount; ++i)
    {
        ImGui::TableNextRow();
        const bool bHighlight = iCurrentStep >= 0 && i == iCurrentStep;
        const ImU32 baseBg = bCfgPreview
            ? ((i & 1) ? IM_COL32(16, 30, 52, 255) : IM_COL32(10, 18, 34, 255))
            : ((i & 1) ? kHudRowStripe : kHudBg);

        ImU32 activeRowBg = bCfgPreview ? IM_COL32(20, 48, 85, 255) : kHudRowActive;
        if (!bCfgPreview && bHighlight)
        {
            const float w = 0.5f + 0.5f * std::sin(static_cast<float>(ImGui::GetTime()) * 2.35f);
            activeRowBg = IM_COL32(14 + static_cast<int>(28.f * w), 38 + static_cast<int>(40.f * w), 68 + static_cast<int>(50.f * w), 255);
        }
        ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, bHighlight ? activeRowBg : baseBg);

        int iCol = 0;
        if (bHudColPop)
        {
            ImGui::TableSetColumnIndex(iCol++);
            if (!bCfgPreview)
                ImGui::AlignTextToFramePadding();
            DrawSupplyCell(vSteps[static_cast<size_t>(i)].iSupply, bCfgPreview ? 1.36f : 1.12f);
        }
        if (bHudColTime)
        {
            ImGui::TableSetColumnIndex(iCol++);
            if (!bCfgPreview)
                ImGui::AlignTextToFramePadding();
            char tbuf[32];
            FormatMmSs(vSteps[static_cast<size_t>(i)].fTimeSec, tbuf, sizeof(tbuf));
            if (bCfgPreview)
                ImGui::Text("%s", tbuf);
            else
                ImGui::TextColored(ImVec4(0.90f, 0.91f, 0.93f, 1.f), "%s", tbuf);
        }
        if (bHudColAction)
        {
            ImGui::TableSetColumnIndex(iCol++);
            if (!bCfgPreview)
                ImGui::AlignTextToFramePadding();
            const std::vector<std::string> vIcons = ResolveStepIconIds(vSteps[static_cast<size_t>(i)], eRace);
            if (m_pIcons != nullptr)
            {
                const ImTextureID unk = m_pIcons->GetUnknownTexture();
                const float fIz = (vIcons.size() > 1u) ? fIconSz * 0.88f : fIconSz;
                for (size_t iIk = 0; iIk < vIcons.size(); ++iIk)
                {
                    if (iIk > 0)
                        ImGui::SameLine(0.f, 3.f);
                    ImTextureID tex = m_pIcons->GetIconTexture(eRace, vIcons[iIk]);
                    const ImVec4 tint = (tex == unk) ? ImVec4(0.55f, 0.58f, 0.72f, 1.f) : ImVec4(1.f, 1.f, 1.f, 1.f);
                    ImGui::ImageWithBg(ImTextureRef(tex), ImVec2(fIz, fIz), ImVec2(0, 0), ImVec2(1, 1), ImVec4(0, 0, 0, 0), tint);
                }
                ImGui::SameLine(0, bCfgPreview ? 8.f : 6.f);
            }
            if (bCfgPreview)
                ImGui::TextWrapped("%s", vSteps[static_cast<size_t>(i)].sNote.c_str());
            else
            {
                ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + 158.f);
                ImGui::TextColored(ImVec4(0.92f, 0.93f, 0.96f, 1.f), "%s", vSteps[static_cast<size_t>(i)].sNote.c_str());
                ImGui::PopTextWrapPos();
            }
        }
        if (bHudColCost)
        {
            ImGui::TableSetColumnIndex(iCol++);
            if (!bCfgPreview)
                ImGui::AlignTextToFramePadding();
            DrawCostCell(vSteps[static_cast<size_t>(i)], m_pIcons);
        }
        if (bHudColNotes)
        {
            ImGui::TableSetColumnIndex(iCol++);
            if (!bCfgPreview)
                ImGui::AlignTextToFramePadding();
            DrawNotesCell(vSteps[static_cast<size_t>(i)].sRowNotes);
        }

        if (bScrollCurrentIntoView && bHighlight)
            ImGui::SetScrollHereY(0.28f);
    }

    ImGui::EndTable();
}

void BuildOrderHud::RenderClockPanel()
{
    constexpr ImGuiChildFlags kCard = ImGuiChildFlags_Borders | ImGuiChildFlags_AlwaysUseWindowPadding | ImGuiChildFlags_AutoResizeY;

    ImGui::PushStyleColor(ImGuiCol_ChildBg, kNeonCard);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 8.f);
    ImGui::BeginChild("clock_section", ImVec2(-1, 0), kCard);

    ImGui::SeparatorText(UiTr(Tr::ClockSectionTitle));

    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(135, 185, 215, 255));
    ImGui::TextWrapped("%s", UiTr(Tr::ClockAlignHint));
    ImGui::PopStyleColor();
    ImGui::Dummy(ImVec2(0.f, 4.f));

    const double dEl = m_clock.GetElapsedSeconds();
    const int iMin = static_cast<int>(dEl / 60.0);
    const int iSec = static_cast<int>(dEl) - iMin * 60;

    ImGui::PushStyleColor(ImGuiCol_ChildBg, kNeonDeep);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 6.f);
    ImGui::BeginChild("time_readout", ImVec2(-1, 54), ImGuiChildFlags_Borders);
    ImGui::SetCursorPos(ImVec2(14.f, 15.f));
    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(115, 205, 255, 255));
    ImGui::SetWindowFontScale(1.22f);
    char timeLine[64];
    std::snprintf(timeLine, sizeof(timeLine), UiTr(Tr::HudTimeLine), iMin, iSec);
    ImGui::TextUnformatted(timeLine);
    ImGui::SetWindowFontScale(1.f);
    ImGui::PopStyleColor();
    ImGui::EndChild();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();

    ImGui::Dummy(ImVec2(0.f, 10.f));

    const float fBtnH = 34.f;
    const float fGap = 10.f;
    const float fRowW = ImGui::GetContentRegionAvail().x;
    const float fCell = (fRowW > fGap * 3.f) ? (fRowW - 3.f * fGap) * 0.25f : (fRowW - fGap) * 0.5f;

    PushHudButtonStylePrimary();
    if (ImGui::Button(UiTr(Tr::BtnStartNow), ImVec2(fCell, fBtnH)))
    {
        m_clock.Reset();
        m_clock.StartNow();
    }
    ImGui::SameLine(0.f, fGap);
    if (ImGui::Button(UiTr(Tr::BtnPause), ImVec2(fCell, fBtnH)))
        m_clock.Pause();
    ImGui::SameLine(0.f, fGap);
    if (ImGui::Button(UiTr(Tr::BtnResume), ImVec2(fCell, fBtnH)))
        m_clock.Resume();
    ImGui::SameLine(0.f, fGap);
    if (ImGui::Button(UiTr(Tr::BtnReset), ImVec2(fCell, fBtnH)))
        m_clock.Reset();
    PopHudButtonStyle();

    ImGui::Dummy(ImVec2(0.f, 12.f));

    static float fManualSec = 0.f;
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(UiTr(Tr::SyncTimeTitle));
    const float fHelpW = 28.f;
    ImGui::SetNextItemWidth((std::max)(120.f, ImGui::GetContentRegionAvail().x - fHelpW));
    ImGui::SliderFloat("##manualt", &fManualSec, 0.f, 3600.f, UiTr(Tr::SliderQuickFmt));
    ImGui::SameLine(0.f, 6.f);
    HelpMarker(UiTr(Tr::HelpClockManual));

    ImGui::Dummy(ImVec2(0.f, 6.f));
    PushHudButtonStyleAccent();
    if (ImGui::Button(UiTr(Tr::BtnApplyTime), ImVec2(-1.f, 38.f)))
        m_clock.SetElapsedSeconds(static_cast<double>(fManualSec));
    PopHudButtonStyle();

    ImGui::Dummy(ImVec2(0.f, 8.f));
    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(92, 130, 158, 255));
    ImGui::TextWrapped("%s", UiTr(Tr::ClockApiFootnote));
    ImGui::PopStyleColor();

    ImGui::EndChild();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
}

void BuildOrderHud::RenderBuildPickerAndViewer()
{
    constexpr ImGuiChildFlags kCard = ImGuiChildFlags_Borders | ImGuiChildFlags_AlwaysUseWindowPadding | ImGuiChildFlags_AutoResizeY;

    ImGui::PushStyleColor(ImGuiCol_ChildBg, kNeonCard);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 8.f);
    ImGui::BeginChild("build_pick_card", ImVec2(-1, 0), kCard);

    ImGui::SeparatorText(UiTr(Tr::BuildPickTitle));
    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(135, 185, 215, 255));
    ImGui::TextWrapped("%s", UiTr(Tr::BuildPickHint));
    ImGui::PopStyleColor();
    ImGui::Dummy(ImVec2(0.f, 4.f));

    PushHudButtonStyleAccent();
    if (ImGui::Button(UiTr(Tr::BtnRefreshList), ImVec2(-1.f, 36.f)))
        m_store.RefreshFileList();
    PopHudButtonStyle();

    const auto& v = m_store.GetKnownBuilds();
    if (!v.empty())
    {
        if (iSelectedList < 0 || iSelectedList >= static_cast<int>(v.size()))
            iSelectedList = 0;
        ImGui::Dummy(ImVec2(0.f, 6.f));
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(UiTr(Tr::BuildSavedLabel));
        ImGui::SetNextItemWidth(-1);
        std::string comboPreview = v[static_cast<size_t>(iSelectedList)].sDisplayName;
        if (ImGui::BeginCombo("##buildlist", comboPreview.c_str()))
        {
            for (int i = 0; i < static_cast<int>(v.size()); ++i)
            {
                const bool bSel = i == iSelectedList;
                if (ImGui::Selectable(v[static_cast<size_t>(i)].sDisplayName.c_str(), bSel))
                    iSelectedList = i;
                if (bSel)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
        ImGui::Dummy(ImVec2(0.f, 6.f));
        PushHudButtonStylePrimary();
        if (ImGui::Button(UiTr(Tr::BtnLoadBuild), ImVec2(-1.f, 36.f)))
        {
            std::string err;
            if (m_store.LoadByRelativePath(v[static_cast<size_t>(iSelectedList)].sRelativePath, m_loaded, err))
            {
                m_editor = m_loaded;
                std::memset(cSaveFileBase, 0, sizeof(cSaveFileBase));
                const std::string base = m_loaded.sName.empty() ? "build" : m_loaded.sName;
                std::snprintf(cSaveFileBase, sizeof(cSaveFileBase), "%s", base.c_str());
                m_iLastOverlayScrollStep = -999;
            }
        }
        PopHudButtonStyle();
    }
    else
    {
        ImGui::Dummy(ImVec2(0.f, 6.f));
        ImGui::TextColored(kHudInfoText, "%s", UiTr(Tr::NoJsonFound));
    }

    ImGui::EndChild();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();

    if (!m_loaded.sName.empty())
    {
        ImGui::Dummy(ImVec2(0.f, 12.f));
        ImGui::SeparatorText(UiTr(Tr::PreviewTitle));
        char ln[384];
        std::snprintf(ln, sizeof(ln), UiTr(Tr::LoadedLabelFmt), m_loaded.sName.c_str());
        ImGui::TextUnformatted(ln);
        char lr[128];
        std::snprintf(lr, sizeof(lr), UiTr(Tr::RaceLabelFmt), RaceToLabel(m_loaded.eRace));
        ImGui::TextUnformatted(lr);
        ImGui::Dummy(ImVec2(0.f, 8.f));

        const float fPreviewH = (std::max)(180.f, ImGui::GetContentRegionAvail().y);
        ImGui::PushStyleColor(ImGuiCol_ChildBg, kNeonDeep);
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 8.f);
        ImGui::BeginChild("build_preview_scroll", ImVec2(0.f, fPreviewH), ImGuiChildFlags_Borders);
        const float fInnerH = ImGui::GetContentRegionAvail().y;
        const double dEl = m_clock.GetElapsedSeconds();
        const int iCur = FindCurrentStepIndex(m_loaded, dEl);
        SetupAndDrawBuildTable(m_loaded.eRace, m_loaded.vSteps, iCur, fInnerH, 26.f, true, false);
        ImGui::EndChild();
        ImGui::PopStyleVar();
        ImGui::PopStyleColor();
    }
}

void BuildOrderHud::RenderEditor()
{
    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(135, 185, 215, 255));
    ImGui::TextWrapped("%s", UiTr(Tr::EditorIntro));
    ImGui::PopStyleColor();
    ImGui::SameLine();
    HelpMarker(UiTr(Tr::HelpCosts));
    ImGui::Dummy(ImVec2(0.f, 10.f));

    if (ImGui::CollapsingHeader(UiTr(Tr::CollImport), ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(135, 185, 215, 255));
        ImGui::TextWrapped("%s", UiTr(Tr::ImportHint));
        ImGui::PopStyleColor();
        ImGui::Dummy(ImVec2(0.f, 6.f));
        PushHudButtonStyleAccent();
        if (ImGui::Button(UiTr(Tr::BtnSpawningToolOpen), ImVec2(-1.f, 32.f)))
            OpenSpawningToolWebsite();
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("%s", UiTr(Tr::TooltipSpawningTool));
        PopHudButtonStyle();
        ImGui::InputTextMultiline("##pastebuild", m_cPasteBuf, sizeof(m_cPasteBuf), ImVec2(-1.f, 130.f));
        ImGui::Dummy(ImVec2(0.f, 6.f));
        PushHudButtonStyleAccent();
        if (ImGui::Button(UiTr(Tr::BtnConvert), ImVec2(0.f, 32.f)))
        {
            std::string sErr;
            BuildOrder imported;
            if (ParseWebsiteBuildOrderPaste(std::string(m_cPasteBuf), imported, sErr))
            {
                m_editor = std::move(imported);
                BuildOrderPasteSuggestedFileBase(m_editor.sName, cSaveFileBase, sizeof(cSaveFileBase));
                m_iLastOverlayScrollStep = -999;
                char msgOk[512];
                std::snprintf(msgOk, sizeof(msgOk), UiTr(Tr::MsgImportOkFmt), m_editor.vSteps.size());
                m_sPasteImportMsg = msgOk;
            }
            else
                m_sPasteImportMsg = sErr;
        }
        ImGui::SameLine(0.f, 10.f);
        if (ImGui::Button(UiTr(Tr::BtnClearPaste), ImVec2(0.f, 32.f)))
        {
            std::memset(m_cPasteBuf, 0, sizeof(m_cPasteBuf));
            m_sPasteImportMsg.clear();
        }
        ImGui::SameLine(0.f, 10.f);
        if (ImGui::Button(UiTr(Tr::BtnCopyJson), ImVec2(0.f, 32.f)))
        {
            std::string sJson;
            std::string sJerr;
            if (BuildOrderToJsonString(m_editor, sJson, sJerr))
            {
                ImGui::SetClipboardText(sJson.c_str());
                char msgJ[384];
                std::snprintf(msgJ, sizeof(msgJ), UiTr(Tr::MsgJsonBytesFmt), sJson.size());
                m_sPasteImportMsg = msgJ;
            }
            else
                m_sPasteImportMsg = sJerr;
        }
        PopHudButtonStyle();
        if (!m_sPasteImportMsg.empty())
        {
            ImGui::Dummy(ImVec2(0.f, 6.f));
            ImGui::TextWrapped("%s", m_sPasteImportMsg.c_str());
        }
    }

    if (ImGui::CollapsingHeader(UiTr(Tr::CollMeta), ImGuiTreeNodeFlags_DefaultOpen))
    {
        char nameBuf[256] = {};
        std::snprintf(nameBuf, sizeof(nameBuf), "%s", m_editor.sName.c_str());
        if (ImGui::InputText(UiTr(Tr::FieldBuildName), nameBuf, sizeof(nameBuf)))
            m_editor.sName = nameBuf;

        int iRace = static_cast<int>(m_editor.eRace);
        if (iRace < 0 || iRace > static_cast<int>(BuildRace::Zerg))
            iRace = static_cast<int>(BuildRace::Unknown);
        const char* rItems[4] = { UiTr(Tr::RaceUnknown), UiTr(Tr::RaceTerran), UiTr(Tr::RaceProtoss), UiTr(Tr::RaceZerg) };
        ImGui::Combo(UiTr(Tr::FieldRace), &iRace, rItems, 4);
        m_editor.eRace = static_cast<BuildRace>(iRace);

        ImGui::Dummy(ImVec2(0.f, 4.f));
        ImGui::InputText(UiTr(Tr::FieldFilename), cSaveFileBase, sizeof(cSaveFileBase));
        ImGui::Dummy(ImVec2(0.f, 8.f));
        PushHudButtonStyleAccent();
        if (ImGui::Button(UiTr(Tr::BtnSaveList), ImVec2(-1.f, 36.f)))
        {
            std::string err;
            if (m_store.SaveUserBuild(m_editor, cSaveFileBase, err))
            {
                m_loaded = m_editor;
                m_iLastOverlayScrollStep = -999;
                m_store.RefreshFileList();
                char msgSv[512];
                std::snprintf(msgSv, sizeof(msgSv), UiTr(Tr::MsgSavedUserFmt), cSaveFileBase);
                m_sPasteImportMsg = msgSv;
            }
            else
                m_sPasteImportMsg = err;
        }
        PopHudButtonStyle();
        ImGui::Dummy(ImVec2(0.f, 6.f));
        PushHudButtonStylePrimary();
        if (ImGui::Button(UiTr(Tr::BtnUseHudNow), ImVec2(-1.f, 32.f)))
        {
            m_loaded = m_editor;
            m_iLastOverlayScrollStep = -999;
        }
        PopHudButtonStyle();
        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(92, 130, 158, 255));
        ImGui::TextWrapped("%s", UiTr(Tr::AfterSaveHint));
        ImGui::PopStyleColor();
    }

    if (ImGui::CollapsingHeader(UiTr(Tr::CollSteps), ImGuiTreeNodeFlags_DefaultOpen))
    {
        PushHudButtonStylePrimary();
        if (ImGui::Button(UiTr(Tr::BtnAddStep), ImVec2(-1.f, 32.f)))
            m_editor.vSteps.push_back(BuildStep{});
        PopHudButtonStyle();
        ImGui::Dummy(ImVec2(0.f, 8.f));

        int iRemove = -1;
        for (int i = 0; i < static_cast<int>(m_editor.vSteps.size()); ++i)
        {
            ImGui::PushID(i);
            const std::string& sNote = m_editor.vSteps[static_cast<size_t>(i)].sNote;
            std::string sPreview = sNote.empty() ? std::string(UiTr(Tr::StepNoDesc)) : sNote;
            if (sPreview.size() > 52)
                sPreview = sPreview.substr(0, 49) + "...";
            const ImGuiTreeNodeFlags tnFlags = ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth;
            char nodeLabel[640];
            std::snprintf(nodeLabel, sizeof(nodeLabel), UiTr(Tr::StepTreeFmt), i + 1, sPreview.c_str());
            if (ImGui::TreeNodeEx((void*)(intptr_t)(i + 1), tnFlags, "%s", nodeLabel))
            {
                int sup = m_editor.vSteps[static_cast<size_t>(i)].iSupply;
                if (ImGui::InputInt(UiTr(Tr::FieldSupply), &sup))
                    m_editor.vSteps[static_cast<size_t>(i)].iSupply = sup;
                float t = m_editor.vSteps[static_cast<size_t>(i)].fTimeSec;
                if (ImGui::InputFloat(UiTr(Tr::FieldTimeSec), &t))
                {
                    m_editor.vSteps[static_cast<size_t>(i)].fTimeSec = (std::max)(0.f, t);
                    m_editor.vSteps[static_cast<size_t>(i)].bTimeSpecified = true;
                }
                char note[512] = {};
                std::snprintf(note, sizeof(note), "%s", m_editor.vSteps[static_cast<size_t>(i)].sNote.c_str());
                if (ImGui::InputText(UiTr(Tr::FieldDescription), note, sizeof(note)))
                {
                    m_editor.vSteps[static_cast<size_t>(i)].sNote = note;
                    StripTrailingParentheticalClauses(m_editor.vSteps[static_cast<size_t>(i)].sNote);
                    m_editor.vSteps[static_cast<size_t>(i)].iQuantity = ParseNoteTrailingBatchQuantity(m_editor.vSteps[static_cast<size_t>(i)].sNote);
                }
                char qtyLine[96];
                std::snprintf(qtyLine, sizeof(qtyLine), UiTr(Tr::QtyAutoFmt), m_editor.vSteps[static_cast<size_t>(i)].iQuantity);
                ImGui::TextDisabled("%s", qtyLine);
                char iconIdBuf[128] = {};
                std::snprintf(iconIdBuf, sizeof(iconIdBuf), "%s", m_editor.vSteps[static_cast<size_t>(i)].sIconId.c_str());
                if (ImGui::InputText(UiTr(Tr::FieldIconId), iconIdBuf, sizeof(iconIdBuf)))
                    m_editor.vSteps[static_cast<size_t>(i)].sIconId = iconIdBuf;
                char rowNotesBuf[512] = {};
                std::snprintf(rowNotesBuf, sizeof(rowNotesBuf), "%s", m_editor.vSteps[static_cast<size_t>(i)].sRowNotes.c_str());
                if (ImGui::InputText(UiTr(Tr::FieldRowNotes), rowNotesBuf, sizeof(rowNotesBuf)))
                    m_editor.vSteps[static_cast<size_t>(i)].sRowNotes = rowNotesBuf;
                ImGui::Dummy(ImVec2(0.f, 4.f));
                if (ImGui::Button(UiTr(Tr::BtnRemoveStep)))
                    iRemove = i;
                ImGui::TreePop();
            }
            ImGui::PopID();
        }
        if (iRemove >= 0 && iRemove < static_cast<int>(m_editor.vSteps.size()))
            m_editor.vSteps.erase(m_editor.vSteps.begin() + iRemove);
    }
}

void BuildOrderHud::RenderMinimalHud()
{
    const float S = m_fOverlayScale;
    constexpr float kHudPanelW = 224.f;

    ImGui::SetNextWindowPos(ImVec2(12.f * S, 12.f * S), ImGuiCond_Always);
    const float fMaxHudH = (std::max)(ImGui::GetIO().DisplaySize.y * 0.52f * S, ImGui::GetIO().DisplaySize.y - 40.f);
    ImGui::SetNextWindowSizeConstraints(ImVec2(kHudPanelW * S, 0.f), ImVec2(kHudPanelW * S, fMaxHudH));
    ImGui::SetNextWindowBgAlpha(1.0f);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.f * S, 6.f * S));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 7.f * S);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4.f * S, 3.f * S));
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 8.f * S);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, kHudBg);
    ImGui::PushStyleColor(ImGuiCol_Border, kHudBorder);
    ImGui::PushStyleColor(ImGuiCol_Separator, kHudSep);
    ImGui::PushStyleColor(ImGuiCol_TableBorderStrong, kHudSep);
    ImGui::PushStyleColor(ImGuiCol_TableBorderLight, kHudSep);

    const ImGuiWindowFlags iFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize
        | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;

    ImGui::Begin("##CompactBuild", nullptr, iFlags);

    {
        const float fBtnW = 26.f * S;
        const float fBtnH = 17.f * S;
        const float fGapBtn = 3.f * S;
        const float fAvailRow = ImGui::GetContentRegionAvail().x;
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (std::max)(0.f, fAvailRow - 2.f * fBtnW - fGapBtn));
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4.f * S, 2.f * S));
        if (ImGui::Button("-##ov_scale", ImVec2(fBtnW, fBtnH)))
        {
            m_fOverlayScale = std::round((m_fOverlayScale - 0.05f) * 20.f) / 20.f;
            m_fOverlayScale = (std::max)(0.5f, m_fOverlayScale);
        }
        if (ImGui::IsItemHovered())
        {
            char tipDn[128];
            std::snprintf(tipDn, sizeof(tipDn), UiTr(Tr::TooltipScaleDownFmt), static_cast<double>(m_fOverlayScale * 100.f));
            ImGui::SetTooltip("%s", tipDn);
        }
        ImGui::SameLine(0.f, fGapBtn);
        if (ImGui::Button("+##ov_scale", ImVec2(fBtnW, fBtnH)))
        {
            m_fOverlayScale = std::round((m_fOverlayScale + 0.05f) * 20.f) / 20.f;
            m_fOverlayScale = (std::min)(2.0f, m_fOverlayScale);
        }
        if (ImGui::IsItemHovered())
        {
            char tipUp[128];
            std::snprintf(tipUp, sizeof(tipUp), UiTr(Tr::TooltipScaleUpFmt), static_cast<double>(m_fOverlayScale * 100.f));
            ImGui::SetTooltip("%s", tipUp);
        }
        ImGui::PopStyleVar();
    }

    if (!m_loaded.vSteps.empty())
    {
        ClampHudColumnsVisible();

        const double dEl = m_clock.GetElapsedSeconds();
        const int iCur = FindCurrentStepIndex(m_loaded, dEl);
        if (iCur != m_iLastOverlayScrollStep)
            m_iLastOverlayScrollStep = iCur;
        const int nSteps = static_cast<int>(m_loaded.vSteps.size());
        constexpr ImU32 kTimeLabel = IM_COL32(130, 175, 205, 255);
        constexpr ImU32 kTimerBright = IM_COL32(0, 215, 255, 255);
        constexpr ImU32 kMutedLine = IM_COL32(88, 118, 142, 255);

        const float fRowAvailW = ImGui::GetContentRegionAvail().x;

        ImGui::PushStyleColor(ImGuiCol_Text, kTimeLabel);
        ImGui::SetWindowFontScale(0.54f * S);
        {
            const ImVec2 sLabel = ImGui::CalcTextSize(UiTr(Tr::ElapsedTimeLabel));
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (fRowAvailW - sLabel.x) * 0.5f);
            ImGui::TextUnformatted(UiTr(Tr::ElapsedTimeLabel));
        }
        ImGui::SetWindowFontScale(S);
        ImGui::PopStyleColor();

        char tbuf[32];
        FormatMmSs(static_cast<float>(dEl), tbuf, sizeof(tbuf));
        ImGui::PushStyleColor(ImGuiCol_Text, kTimerBright);
        ImGui::SetWindowFontScale(1.14f * S);
        {
            const ImVec2 sTime = ImGui::CalcTextSize(tbuf);
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (fRowAvailW - sTime.x) * 0.5f);
            ImGui::Text("%s", tbuf);
        }
        ImGui::SetWindowFontScale(S);
        ImGui::PopStyleColor();

        if (!m_loaded.sName.empty())
        {
            ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(135, 185, 215, 255));
            ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + ImGui::GetContentRegionAvail().x);
            ImGui::TextWrapped("%s", m_loaded.sName.c_str());
            ImGui::PopTextWrapPos();
            ImGui::PopStyleColor();
        }

        ImGui::Spacing();
        {
            const float fGap = 5.f * S;
            const float fAvail = ImGui::GetContentRegionAvail().x;
            const float fPad = 3.f * S;
            constexpr float kMaxHudControlImg = 22.f;
            const float fHalf = std::floor((fAvail - fGap) * 0.5f);
            const float fImg = (std::min)(kMaxHudControlImg * S, (std::max)(14.f * S, fHalf - 2.f * fPad));

            if (m_pIcons != nullptr)
            {
                const ImTextureID texPlay = m_pIcons->GetUiIconTexture("play");
                const ImTextureID texPause = m_pIcons->GetUiIconTexture("pause");
                const ImTextureID texRestart = m_pIcons->GetUiIconTexture("restart");
                const ImTextureID texMain = m_clock.IsRunning() ? texPause : texPlay;

                const float fBlockW = 2.f * (fImg + 2.f * fPad) + fGap + ImGui::GetStyle().FrameBorderSize * 4.f;
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (std::max)(0.f, (fAvail - fBlockW) * 0.5f));

                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(fPad, fPad));
                ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(12, 45, 82, 255));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(0, 105, 160, 255));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(0, 75, 118, 255));

                const ImVec2 imgSz(fImg, fImg);
                if (ImGui::ImageButton("##hud_reset", ImTextureRef(texRestart), imgSz))
                {
                    m_clock.Reset();
                    m_clock.StartNow();
                }
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("%s", UiTr(Tr::TooltipResetTime));

                ImGui::SameLine(0.f, fGap);

                if (ImGui::ImageButton("##hud_run", ImTextureRef(texMain), imgSz))
                {
                    if (m_clock.IsRunning())
                        m_clock.Pause();
                    else
                        m_clock.Resume();
                }
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("%s", m_clock.IsRunning() ? UiTr(Tr::TooltipPause) : UiTr(Tr::TooltipResume));

                ImGui::PopStyleColor(3);
                ImGui::PopStyleVar();
            }
            else
            {
                const float fBtnH = 28.f * S;
                const float fW = (std::min)(52.f * S, (std::max)(30.f * S, std::floor((fAvail - fGap) * 0.5f)));
                if (ImGui::Button("\xe2\x86\xbb", ImVec2(fW, fBtnH)))
                {
                    m_clock.Reset();
                    m_clock.StartNow();
                }
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("%s", UiTr(Tr::TooltipResetTime));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(0, 170, 220, 255));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(0, 120, 175, 255));
                if (m_clock.IsRunning())
                {
                    if (ImGui::Button(UiTr(Tr::BtnPauseEn), ImVec2(fW, fBtnH)))
                        m_clock.Pause();
                }
                else
                {
                    if (ImGui::Button(UiTr(Tr::BtnResumeEn), ImVec2(fW, fBtnH)))
                        m_clock.Resume();
                }
                ImGui::PopStyleColor(2);
            }
        }

        ImGui::Spacing();
        ImGui::Spacing();

        if (iCur >= 0 && iCur < nSteps)
        {
            const BuildStep& st = m_loaded.vSteps[static_cast<size_t>(iCur)];
            const float fPulse = 0.5f + 0.5f * std::sin(static_cast<float>(ImGui::GetTime()) * 2.2f);

            ImGui::PushStyleColor(ImGuiCol_ChildBg, kNeonCard);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(9.f * S, 8.f * S));
            const float fCardW = (std::max)(100.f * S, ImGui::GetContentRegionAvail().x);
            ImGui::BeginChild("FollowCurStep", ImVec2(fCardW, 0.f), ImGuiChildFlags_AlwaysUseWindowPadding | ImGuiChildFlags_AutoResizeY, ImGuiWindowFlags_NoScrollbar);

            if (iCur > 0)
            {
                const int iHistBegin = (std::max)(0, iCur - 3);
                bool bDrewAnyHist = false;
                for (int ih = iHistBegin; ih < iCur; ++ih)
                {
                    const BuildStep& prev = m_loaded.vSteps[static_cast<size_t>(ih)];
                    const bool bPrevShowSupply = bHudColPop && prev.iSupply >= 0;
                    const bool bPrevShowIcons = m_pIcons != nullptr;
                    const bool bPrevShowTime = bHudColTime && prev.bTimeSpecified;
                    const bool bPrevShowName = bHudColAction && !prev.sNote.empty();
                    if (!bPrevShowIcons && !bPrevShowSupply && !bPrevShowTime && !bPrevShowName)
                        continue;
                    if (bDrewAnyHist)
                        ImGui::Spacing();
                    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.78f);
                    ImGui::PushStyleColor(ImGuiCol_Text, kMutedLine);
                    ImGui::SetWindowFontScale(0.78f * S);
                    if (bPrevShowIcons)
                    {
                        const std::vector<std::string> vPrevIcons = ResolveStepIconIds(prev, m_loaded.eRace);
                        const ImTextureID unk = m_pIcons->GetUnknownTexture();
                        const float fPrevIcon = 18.f * S;
                        const float fIz = (vPrevIcons.size() > 1u) ? fPrevIcon * 0.88f : fPrevIcon;
                        for (size_t iIk = 0; iIk < vPrevIcons.size(); ++iIk)
                        {
                            if (iIk > 0)
                                ImGui::SameLine(0.f, 3.f * S);
                            ImTextureID tex = m_pIcons->GetIconTexture(m_loaded.eRace, vPrevIcons[iIk]);
                            const ImVec4 tint = (tex == unk) ? ImVec4(0.55f, 0.58f, 0.72f, 1.f) : ImVec4(1.f, 1.f, 1.f, 1.f);
                            ImGui::ImageWithBg(ImTextureRef(tex), ImVec2(fIz, fIz), ImVec2(0.f, 0.f), ImVec2(1.f, 1.f), ImVec4(0.f, 0.f, 0.f, 0.f), tint);
                        }
                    }
                    bool bNeedSameLine = bPrevShowIcons;
                    char histTime[32] = {};
                    if (bPrevShowTime)
                        FormatMmSs(prev.fTimeSec, histTime, sizeof(histTime));
                    float fSupTextW = 0.f;
                    if (bPrevShowSupply)
                    {
                        char sTmp[16];
                        std::snprintf(sTmp, sizeof(sTmp), "%d", prev.iSupply);
                        fSupTextW = ImGui::CalcTextSize(sTmp).x;
                    }
                    if (bPrevShowTime)
                    {
                        if (bNeedSameLine)
                            ImGui::SameLine(0.f, 6.f * S);
                        bNeedSameLine = true;
                        ImGui::AlignTextToFramePadding();
                        ImGui::TextUnformatted(histTime);
                    }
                    if (bPrevShowName)
                    {
                        if (bNeedSameLine)
                            ImGui::SameLine(0.f, 6.f * S);
                        bNeedSameLine = true;
                        ImGui::AlignTextToFramePadding();
                        const float fAvailNow = ImGui::GetContentRegionAvail().x;
                        const float fReserveSup = bPrevShowSupply ? fSupTextW + 10.f * S : 0.f;
                        const float fNameMax = (std::max)(24.f * S, fAvailNow - fReserveSup);
                        TextHudHistLineClipped(prev.sNote.c_str(), fNameMax);
                    }
                    if (bPrevShowSupply)
                    {
                        if (bNeedSameLine)
                            ImGui::SameLine(0.f, 6.f * S);
                        ImGui::AlignTextToFramePadding();
                        ImGui::Text("%d", prev.iSupply);
                    }
                    ImGui::SetWindowFontScale(S);
                    ImGui::PopStyleColor();
                    ImGui::PopStyleVar();
                    bDrewAnyHist = true;
                }
                if (bDrewAnyHist)
                {
                    ImGui::Spacing();
                    ImGui::Separator();
                    ImGui::Spacing();
                }
            }

            ImGui::PushStyleColor(ImGuiCol_Text, kHudAccent);
            ImGui::SetWindowFontScale(0.55f * S);
            char hdr[72];
            std::snprintf(hdr, sizeof(hdr), UiTr(Tr::CurrentStepFmt), iCur + 1, nSteps);
            ImGui::TextUnformatted(hdr);
            ImGui::SetWindowFontScale(S);
            ImGui::PopStyleColor();

            ImGui::Spacing();
            if (bHudColAction || m_pIcons != nullptr)
            {
                if (m_pIcons != nullptr)
                {
                    const std::vector<std::string> vIcons = ResolveStepIconIds(st, m_loaded.eRace);
                    const ImTextureID unk = m_pIcons->GetUnknownTexture();
                    const float fIzCur = ((vIcons.size() > 1u) ? 24.f : 28.f) * S;
                    for (size_t iIk = 0; iIk < vIcons.size(); ++iIk)
                    {
                        if (iIk > 0)
                            ImGui::SameLine(0.f, 6.f * S);
                        ImTextureID tex = m_pIcons->GetIconTexture(m_loaded.eRace, vIcons[iIk]);
                        const ImVec4 tint = (tex == unk) ? ImVec4(0.55f, 0.58f, 0.72f, 1.f) : ImVec4(1.f, 1.f, 1.f, 1.f);
                        ImGui::ImageWithBg(ImTextureRef(tex), ImVec2(fIzCur, fIzCur), ImVec2(0.f, 0.f), ImVec2(1.f, 1.f), ImVec4(0.f, 0.f, 0.f, 0.f), tint);
                    }
                    ImGui::SameLine(0.f, 8.f * S);
                }
                if (bHudColAction)
                {
                    ImGui::AlignTextToFramePadding();
                    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(245, 248, 252, 255));
                    ImGui::SetWindowFontScale(0.95f * S);
                    ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + (std::max)(60.f * S, ImGui::GetContentRegionAvail().x));
                    ImGui::Text("%s", st.sNote.c_str());
                    ImGui::PopTextWrapPos();
                    ImGui::SetWindowFontScale(S);
                    ImGui::PopStyleColor();
                }
            }

            if (bHudColNotes && !st.sRowNotes.empty())
            {
                ImGui::Spacing();
                ImGui::PushStyleColor(ImGuiCol_Text, kMutedLine);
                ImGui::SetWindowFontScale(0.78f * S);
                ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + (std::max)(60.f * S, ImGui::GetContentRegionAvail().x));
                ImGui::TextWrapped("%s", st.sRowNotes.c_str());
                ImGui::PopTextWrapPos();
                ImGui::SetWindowFontScale(S);
                ImGui::PopStyleColor();
            }

            if (bHudColPop && st.iSupply >= 0)
            {
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();
                ImGui::PushStyleColor(ImGuiCol_Text, kTimerBright);
                ImGui::SetWindowFontScale(1.22f * S);
                char sup[16];
                std::snprintf(sup, sizeof(sup), "%d", st.iSupply);
                ImGui::Text("%s", sup);
                ImGui::SetWindowFontScale(S);
                ImGui::PopStyleColor();
                ImGui::SameLine(0.f, 8.f * S);
                ImGui::AlignTextToFramePadding();
                ImGui::PushStyleColor(ImGuiCol_Text, kTimeLabel);
                ImGui::SetWindowFontScale(0.58f * S);
                ImGui::TextUnformatted(UiTr(Tr::SupplyLabel));
                ImGui::SetWindowFontScale(S);
                ImGui::PopStyleColor();
            }

            if (bHudColCost && (st.iMinerals >= 0 || st.iGas >= 0 || st.iBuildTimeSec >= 0))
            {
                ImGui::Spacing();
                DrawCostCell(st, m_pIcons, S);
            }

            ImGui::EndChild();
            const ImVec2 cardRectMin = ImGui::GetItemRectMin();
            const ImVec2 cardRectMax = ImGui::GetItemRectMax();
            DrawFollowAlongCardGlow(ImGui::GetWindowDrawList(), cardRectMin, cardRectMax, 8.f * S, fPulse);
            ImGui::PopStyleVar();
            ImGui::PopStyleColor();
        }

        if (iCur + 1 < nSteps)
        {
            ImGui::Spacing();
            ImGui::Spacing();
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(5.f * S, 5.f * S));
            if (ImGui::ArrowButton("toggle_coming_up", m_bShowComingUp ? ImGuiDir_Down : ImGuiDir_Right))
                m_bShowComingUp = !m_bShowComingUp;
            ImGui::PopStyleVar();
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("%s", m_bShowComingUp ? UiTr(Tr::TooltipComingUpHide) : UiTr(Tr::TooltipComingUpShow));
            ImGui::SameLine(0.f, 6.f * S);
            ImGui::PushStyleColor(ImGuiCol_Text, kTimeLabel);
            ImGui::SetWindowFontScale(0.54f * S);
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted(UiTr(Tr::ComingUpTitle));
            ImGui::SetWindowFontScale(S);
            ImGui::PopStyleColor();

            if (m_bShowComingUp)
            {
                ImGui::Spacing();

                const int nUpcoming = nSteps - (iCur + 1);
                const float fRowH = 24.f * S;
                const float listH = (std::min)((std::min)(170.f * S, ImGui::GetIO().DisplaySize.y * 0.22f * S), (std::max)(40.f * S, fRowH * static_cast<float>(nUpcoming) + 14.f * S));

                ImGui::PushStyleColor(ImGuiCol_ChildBg, kNeonDeep);
                const float fListW = (std::max)(100.f * S, ImGui::GetContentRegionAvail().x);
                ImGui::BeginChild("ComingUpList", ImVec2(fListW, listH), ImGuiChildFlags_Borders, ImGuiWindowFlags_None);

                const float fUpIcon = 20.f * S;
                const float fMineralIcon = 13.f * S;
                for (int r = iCur + 1; r < nSteps; ++r)
                {
                    const BuildStep& rw = m_loaded.vSteps[static_cast<size_t>(r)];
                    ImGui::PushID(r);

                    char syncTimeBuf[32] = {};
                    if (rw.bTimeSpecified)
                        FormatMmSs(rw.fTimeSec, syncTimeBuf, sizeof(syncTimeBuf));

                    const ImVec2 vRowScreen0 = ImGui::GetCursorScreenPos();
                    const bool bShowSupply = rw.iSupply >= 0;
                    const bool bShowTime = rw.bTimeSpecified;
                    const bool bShowMn = rw.iMinerals >= 0;
                    const bool bShowGas = rw.iGas >= 0;

                    ImGui::BeginGroup();

                    if (bShowSupply)
                    {
                        ImGui::AlignTextToFramePadding();
                        ImGui::PushStyleColor(ImGuiCol_Text, kTimerBright);
                        ImGui::Text("%d", rw.iSupply);
                        ImGui::PopStyleColor();
                        const bool bHasMore = (m_pIcons != nullptr) || bShowTime || bShowMn || bShowGas;
                        if (bHasMore)
                            ImGui::SameLine(0.f, 6.f * S);
                    }

                    if (m_pIcons != nullptr)
                    {
                        const std::vector<std::string> vIcons = ResolveStepIconIds(rw, m_loaded.eRace);
                        const ImTextureID unk = m_pIcons->GetUnknownTexture();
                        const float fIz = (vIcons.size() > 1u) ? fUpIcon * 0.88f : fUpIcon;
                        for (size_t iIk = 0; iIk < vIcons.size(); ++iIk)
                        {
                            if (iIk > 0)
                                ImGui::SameLine(0.f, 4.f * S);
                            ImTextureID tex = m_pIcons->GetIconTexture(m_loaded.eRace, vIcons[iIk]);
                            const ImVec4 tint = (tex == unk) ? ImVec4(0.55f, 0.58f, 0.72f, 1.f) : ImVec4(1.f, 1.f, 1.f, 1.f);
                            ImGui::ImageWithBg(ImTextureRef(tex), ImVec2(fIz, fIz), ImVec2(0.f, 0.f), ImVec2(1.f, 1.f), ImVec4(0.f, 0.f, 0.f, 0.f), tint);
                        }
                        ImGui::SameLine(0.f, 7.f * S);
                    }
                    if (bShowTime)
                    {
                        ImGui::AlignTextToFramePadding();
                        ImGui::PushStyleColor(ImGuiCol_Text, kMutedLine);
                        ImGui::Text("%s", syncTimeBuf);
                        ImGui::PopStyleColor();
                        if (bShowMn || bShowGas)
                            ImGui::SameLine(0.f, 12.f * S);
                    }

                    if (m_pIcons != nullptr && (bShowMn || bShowGas))
                    {
                        if (!bShowTime)
                            ImGui::AlignTextToFramePadding();
                        const ImTextureID unkR = m_pIcons->GetUnknownTexture();
                        if (bShowMn)
                        {
                            ImTextureID mTex = m_pIcons->GetIconTexture(BuildRace::Unknown, "minerals");
                            const ImVec4 tintM = (mTex == unkR) ? ImVec4(0.5f, 0.55f, 0.72f, 1.f) : ImVec4(1.f, 1.f, 1.f, 1.f);
                            ImGui::ImageWithBg(ImTextureRef(mTex), ImVec2(fMineralIcon, fMineralIcon), ImVec2(0.f, 0.f), ImVec2(1.f, 1.f), ImVec4(0.f, 0.f, 0.f, 0.f), tintM);
                            ImGui::SameLine(0.f, 5.f * S);
                            ImGui::AlignTextToFramePadding();
                            ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(115, 205, 255, 255));
                            ImGui::Text("%d", rw.iMinerals);
                            ImGui::PopStyleColor();
                        }
                        if (bShowGas)
                        {
                            if (bShowMn)
                                ImGui::SameLine(0.f, 12.f * S);
                            ImTextureID gTex = m_pIcons->GetIconTexture(BuildRace::Unknown, "gas");
                            const ImVec4 tintG = (gTex == unkR) ? ImVec4(0.5f, 0.55f, 0.72f, 1.f) : ImVec4(1.f, 1.f, 1.f, 1.f);
                            ImGui::ImageWithBg(ImTextureRef(gTex), ImVec2(fMineralIcon, fMineralIcon), ImVec2(0.f, 0.f), ImVec2(1.f, 1.f), ImVec4(0.f, 0.f, 0.f, 0.f), tintG);
                            ImGui::SameLine(0.f, 5.f * S);
                            ImGui::AlignTextToFramePadding();
                            ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(120, 255, 160, 255));
                            ImGui::Text("%d", rw.iGas);
                            ImGui::PopStyleColor();
                        }
                    }
                    else if (m_pIcons == nullptr && (bShowMn || bShowGas))
                    {
                        if (!bShowTime)
                            ImGui::AlignTextToFramePadding();
                        if (bShowMn)
                        {
                            ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(115, 205, 255, 255));
                            ImGui::Text("%d M", rw.iMinerals);
                            ImGui::PopStyleColor();
                        }
                        if (bShowGas)
                        {
                            if (bShowMn)
                                ImGui::SameLine(0.f, 10.f * S);
                            ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(120, 255, 160, 255));
                            ImGui::Text("%d G", rw.iGas);
                            ImGui::PopStyleColor();
                        }
                    }

                    ImGui::EndGroup();

                    {
                        const ImVec2 vRowBr = ImGui::GetItemRectMax();
                        const ImVec2 vWp = ImGui::GetWindowPos();
                        const ImVec2 vCrMax = ImGui::GetWindowContentRegionMax();
                        const float fHitR = vWp.x + vCrMax.x;
                        const ImVec2 vHitMax(fHitR, vRowBr.y);
                        const bool bHit = ImGui::IsMouseHoveringRect(vRowScreen0, vHitMax, true);
                        const bool bCanSync = rw.bTimeSpecified;
                        const bool bTipNotes = bHit && bHudColNotes && !rw.sRowNotes.empty();
                        if (bHit && bCanSync)
                            ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
                        if (bHit)
                        {
                            char tipN[768];
                            char tipS[256];
                            if (bTipNotes && bCanSync)
                            {
                                std::snprintf(tipN, sizeof(tipN), UiTr(Tr::TooltipSyncNotesTimeFmt), rw.sRowNotes.c_str(), syncTimeBuf);
                                ImGui::SetTooltip("%s", tipN);
                            }
                            else if (bTipNotes)
                                ImGui::SetTooltip("%s", rw.sRowNotes.c_str());
                            else if (bCanSync)
                            {
                                std::snprintf(tipS, sizeof(tipS), UiTr(Tr::TooltipSyncTimeFmt), syncTimeBuf);
                                ImGui::SetTooltip("%s", tipS);
                            }
                            if (bCanSync && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
                                m_clock.SetElapsedSeconds(static_cast<double>(rw.fTimeSec));
                        }
                    }

                    if (r + 1 < nSteps)
                        ImGui::Separator();
                    ImGui::PopID();
                }

                ImGui::EndChild();
                ImGui::PopStyleColor();
            }
        }

        ImGui::Spacing();
        ImGui::SetWindowFontScale(S);
        ImGui::TextColored(ImVec4(0.48f, 0.68f, 0.88f, 1.f), "%s", UiTr(Tr::FooterInsertConfig));
    }
    else
    {
        ImGui::SetWindowFontScale(S);
        ImGui::TextColored(kHudInfoText, "%s", UiTr(Tr::NoBuildLoaded));
    }

    ImGui::End();

    ImGui::PopStyleColor(5);
    ImGui::PopStyleVar(4);
}

void BuildOrderHud::RenderConfigPanel()
{
    ImGui::SetNextWindowSize(ImVec2(680, 820), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImVec2(48.f, 48.f), ImGuiCond_FirstUseEver);

    if (!ImGui::Begin(UiTr(Tr::ConfigTitle), &bConfigMenuOpen, ImGuiWindowFlags_None))
    {
        ImGui::End();
        return;
    }

    PushConfigPanelSpacing();
    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(135, 185, 215, 255));
    ImGui::TextWrapped("%s", UiTr(Tr::ConfigCloseHint));
    ImGui::PopStyleColor();
    ImGui::Dummy(ImVec2(0.f, 8.f));

    constexpr ImGuiChildFlags kCard = ImGuiChildFlags_Borders | ImGuiChildFlags_AlwaysUseWindowPadding | ImGuiChildFlags_AutoResizeY;
    ImGui::PushStyleColor(ImGuiCol_ChildBg, kNeonCard);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 8.f);
    ImGui::BeginChild("cfg_intro", ImVec2(-1, 0), kCard);
    ImGui::PushStyleColor(ImGuiCol_Text, kHudAccent);
    ImGui::TextUnformatted(UiTr(Tr::ShortcutsTitle));
    ImGui::PopStyleColor();
    ImGui::BulletText("%s", UiTr(Tr::ShortcutInsert));
    ImGui::BulletText("%s", UiTr(Tr::ShortcutEnd));
    ImGui::EndChild();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
    ImGui::Dummy(ImVec2(0.f, 10.f));

    ImGui::PushStyleColor(ImGuiCol_ChildBg, kNeonCard);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 8.f);
    ImGui::BeginChild("cfg_lang", ImVec2(-1, 0), kCard);
    ImGui::TextUnformatted(UiTr(Tr::LangLabel));
    int iLangCombo = (UiGetLang() == UiLang::PortugueseBR) ? 1 : 0;
    const char* langItems[] = { "English", "Portugues (Brasil)" };
    ImGui::SetNextItemWidth(-1);
    if (ImGui::Combo("##settingslang", &iLangCombo, langItems, 2))
    {
        UiSetLang(iLangCombo == 1 ? UiLang::PortugueseBR : UiLang::English);
        UiLanguageSaveToDisk();
    }
    ImGui::EndChild();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
    ImGui::Dummy(ImVec2(0.f, 10.f));

    if (ImGui::BeginTabBar("ConfigTabBar", ImGuiTabBarFlags_FittingPolicyScroll))
    {
        if (ImGui::BeginTabItem(UiTr(Tr::TabMatch)))
        {
            RenderClockPanel();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem(UiTr(Tr::TabBuilds)))
        {
            RenderBuildPickerAndViewer();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem(UiTr(Tr::TabHud)))
        {
            ImGui::BeginChild("hud_tab_body", ImVec2(0.f, ImGui::GetContentRegionAvail().y), ImGuiChildFlags_None, ImGuiWindowFlags_NoBackground);
            ImGui::PushStyleColor(ImGuiCol_ChildBg, kNeonCard);
            ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 8.f);
            ImGui::BeginChild("hud_info_card", ImVec2(-1, 0), kCard);
            ImGui::SeparatorText(UiTr(Tr::HudTabSectionTitle));
            ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(135, 185, 215, 255));
            ImGui::TextWrapped("%s", UiTr(Tr::HudTabBody));
            ImGui::PopStyleColor();
            ImGui::Dummy(ImVec2(0.f, 6.f));
            ImGui::BulletText("%s", UiTr(Tr::HudBulletCommon));
            ImGui::BulletText("%s", UiTr(Tr::HudBulletRace));
            ImGui::EndChild();
            ImGui::PopStyleVar();
            ImGui::PopStyleColor();
            ImGui::Dummy(ImVec2(0.f, 12.f));
            RenderHudColumnToggles();
            ImGui::EndChild();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem(UiTr(Tr::TabEditor)))
        {
            ImGui::BeginChild("editor_tab_scroll", ImVec2(0.f, ImGui::GetContentRegionAvail().y), ImGuiChildFlags_None, ImGuiWindowFlags_AlwaysVerticalScrollbar);
            RenderEditor();
            ImGui::EndChild();
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

    PopConfigPanelSpacing();
    ImGui::End();
}

void BuildOrderHud::RenderFrame()
{
    ApplySc2AutoCostsToBuildSteps(m_loaded.eRace, m_loaded.vSteps);
    ApplySc2AutoCostsToBuildSteps(m_editor.eRace, m_editor.vSteps);

    if (bConfigMenuOpen)
        RenderConfigPanel();
    else
        RenderMinimalHud();
}
