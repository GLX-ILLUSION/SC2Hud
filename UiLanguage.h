#pragma once

enum class UiLang : int
{
    English = 0,
    PortugueseBR = 1,
};

/** Application UI strings (index must match `kUi` row order in UiLanguage.cpp). */
enum class Tr : int
{
    // Settings panel — title and shortcuts
    ConfigTitle,
    ConfigCloseHint,
    ShortcutsTitle,
    ShortcutInsert,
    ShortcutEnd,

    // Settings — language
    LangLabel,
    LangEnglish,
    LangPortuguesBr,

    // Main settings tabs
    TabMatch,
    TabBuilds,
    TabHud,
    TabEditor,

    // Match tab — game clock
    ClockSectionTitle,
    ClockAlignHint,
    HudTimeLine,
    BtnStartNow,
    BtnPause,
    BtnResume,
    BtnReset,
    SyncTimeTitle,
    SliderQuickFmt,
    BtnApplyTime,
    ClockApiFootnote,
    HelpClockManual,

    // Builds tab — list and preview
    BuildPickTitle,
    BuildPickHint,
    BtnRefreshList,
    BuildSavedLabel,
    BtnLoadBuild,
    NoJsonFound,
    PreviewTitle,
    LoadedLabelFmt,
    RaceLabelFmt,

    // Editor tab — import, meta, steps
    EditorIntro,
    HelpCosts,
    CollImport,
    ImportHint,
    BtnSpawningToolOpen,
    TooltipSpawningTool,
    BtnConvert,
    BtnClearPaste,
    BtnCopyJson,
    FieldSpawningToolUrl,
    BtnImportFromUrl,
    TooltipImportUrl,
    CollMeta,
    FieldBuildName,
    FieldRace,
    FieldFilename,
    BtnSaveList,
    BtnUseHudNow,
    AfterSaveHint,
    CollSteps,
    BtnAddStep,
    StepTreeFmt,
    FieldSupply,
    FieldTimeSec,
    FieldDescription,
    QtyAutoFmt,
    FieldIconId,
    FieldRowNotes,
    BtnRemoveStep,

    // Race names (shared)
    RaceUnknown,
    RaceTerran,
    RaceProtoss,
    RaceZerg,

    // Table / HUD columns
    ColPop,
    ColTime,
    ColAction,
    ColCost,
    ColNotes,
    ColVisibleTitle,
    ColVisibleHint,
    HelpColMarker,

    // HUD tab — help copy
    HudTabSectionTitle,
    HudTabBody,
    HudBulletCommon,
    HudBulletRace,

    // In-game overlay
    ElapsedTimeLabel,
    TooltipScaleDownFmt,
    TooltipScaleUpFmt,
    TooltipResetTime,
    TooltipPause,
    TooltipResume,
    BtnPauseEn,
    BtnResumeEn,
    CurrentStepFmt,
    SupplyLabel,
    TooltipComingUpHide,
    TooltipComingUpShow,
    ComingUpTitle,
    FooterInsertConfig,
    NoBuildLoaded,
    TooltipSyncNotesTimeFmt,
    TooltipSyncTimeFmt,

    // Status messages
    MsgImportOkFmt,
    MsgSavedUserFmt,
    MsgJsonBytesFmt,

    StepNoDesc,
    Count
};

void UiLanguageLoadFromDisk();
void UiLanguageSaveToDisk();

UiLang UiGetLang();
void UiSetLang(UiLang lang);

const char* UiTr(Tr id);
