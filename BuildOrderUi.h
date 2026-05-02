#pragma once

#include "BuildOrderStore.h"
#include "BuildOrderTypes.h"
#include "MatchClock.h"

#include "imgui.h"

class IconTextureCache;

class BuildOrderHud
{
public:
    explicit BuildOrderHud(BuildOrderStore& store, MatchClock& matchClock);

    void ToggleConfigMenu() { bConfigMenuOpen = !bConfigMenuOpen; }
    bool IsConfigMenuOpen() const { return bConfigMenuOpen; }

    void SetIconCache(IconTextureCache* pCache) { m_pIcons = pCache; }

    void RenderFrame();

private:
    BuildOrderStore& m_store;
    MatchClock& m_clock;

    IconTextureCache* m_pIcons = nullptr;

    bool bConfigMenuOpen = true;

    BuildOrder m_loaded;
    BuildOrder m_editor;

    int iSelectedList = -1;
    int m_iLastOverlayScrollStep = -999;
    bool m_bShowComingUp = true;

    // Escala da overlay compacta (INSERT fechado): 0.5 = 50%, 1 = 100%, 2 = 200%; passos de 5%.
    float m_fOverlayScale = 1.f;

    char m_cPasteBuf[32768] = {};
    std::string m_sPasteImportMsg;

    char cSaveFileBase[128] = "minha_build";

    bool bHudColPop = true;
    bool bHudColTime = true;
    bool bHudColAction = true;
    bool bHudColCost = true;
    bool bHudColNotes = true;

    void RenderHudColumnToggles();
    void ClampHudColumnsVisible();
    int CountHudColumns() const;
    void SetupAndDrawBuildTable(BuildRace eRace, const std::vector<BuildStep>& vSteps, int iCurrentStep, float fTableHeight, float fIconSz, bool bCfgPreview, bool bScrollCurrentIntoView);
    void RenderConfigPanel();
    void RenderMinimalHud();
    void RenderClockPanel();
    void RenderBuildPickerAndViewer();
    void RenderEditor();

    static const char* RaceToLabel(BuildRace eRace);
    static void FormatMmSs(float fTimeSec, char* cBuf, size_t nBuf);
};
