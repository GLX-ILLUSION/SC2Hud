#pragma once

#include "BuildOrderTypes.h"

#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

#include <d3d9.h>

#include "imgui.h"

class IconTextureCache
{
public:
    void SetDevice(IDirect3DDevice9* pDevice) { m_pDevice = pDevice; }
    void SetBaseDirectory(std::filesystem::path exeDirectory);

    void InvalidateAll();

    ImTextureID GetIconTexture(BuildRace eRace, const std::string& sIconId);
    ImTextureID GetUiIconTexture(const std::string& sName);

    ImTextureID GetUnknownTexture();

private:
    IDirect3DDevice9* m_pDevice = nullptr;
    std::filesystem::path m_exeDir;
    std::vector<std::filesystem::path> m_vSearchRoots;

    IDirect3DTexture9* m_pUnknownTint = nullptr;

    std::unordered_map<std::string, IDirect3DTexture9*> m_loadedByKey;

    static std::string MakeKey(BuildRace eRace, const std::string& sIconId);
    static const char* RaceSubdir(BuildRace eRace);

    IDirect3DTexture9* LoadPngFile(const std::filesystem::path& path);
    void RebuildSearchRoots();
    static IDirect3DTexture9* CreateSolidTexture(IDirect3DDevice9* pDevice, int iW, int iH, D3DCOLOR color);
    bool EnsureUnknownTint();
};
