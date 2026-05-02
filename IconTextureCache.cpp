#include "IconTextureCache.h"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <cstdint>
#include <fstream>
#include <vector>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#include <objidl.h>
#include <wincodec.h>

#pragma comment(lib, "windowscodecs.lib")
#pragma comment(lib, "ole32.lib")

namespace
{
ImTextureID D3D9TexToImId(IDirect3DTexture9* pTex)
{
    return static_cast<ImTextureID>(reinterpret_cast<intptr_t>(pTex));
}

std::filesystem::path FindIconPngInDirectoryImpl(const std::filesystem::path& iconDir, const std::string& idLower)
{
    std::error_code ec;
    if (!std::filesystem::is_directory(iconDir, ec) || ec)
        return {};
    const std::string wantFile = idLower + ".png";
    std::vector<std::filesystem::path> vMatch;
    for (const auto& entry : std::filesystem::directory_iterator(iconDir, ec))
    {
        if (ec)
            break;
        if (!entry.is_regular_file())
            continue;
        std::string fname = entry.path().filename().string();
        std::string fLower = fname;
        std::transform(fLower.begin(), fLower.end(), fLower.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        if (fLower == wantFile)
            vMatch.push_back(entry.path());
    }
    if (vMatch.empty())
        return {};
    if (vMatch.size() == 1)
        return vMatch.front();

    const std::string preferExact = idLower + ".png";
    for (const auto& p : vMatch)
    {
        if (p.filename().string() == preferExact)
            return p;
    }
    std::string sTitle = idLower;
    if (!sTitle.empty())
        sTitle[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(sTitle[0])));
    const std::string preferTitle = sTitle + ".png";
    for (const auto& p : vMatch)
    {
        if (p.filename().string() == preferTitle)
            return p;
    }
    std::sort(vMatch.begin(), vMatch.end());
    return vMatch.front();
}

// nomes de packs SC2/comunidade: terran_hellion.png, btn-unit-terran-hellion.png (termina em _hellion ou -hellion)
std::filesystem::path FindIconPngByIdSuffixInDirectoryImpl(const std::filesystem::path& iconDir, const std::string& idLower)
{
    std::error_code ec;
    if (!std::filesystem::is_directory(iconDir, ec) || ec)
        return {};
    if (idLower.empty())
        return {};
    for (const auto& entry : std::filesystem::directory_iterator(iconDir, ec))
    {
        if (ec)
            break;
        if (!entry.is_regular_file())
            continue;
        std::string fname = entry.path().filename().string();
        std::string fLower = fname;
        std::transform(fLower.begin(), fLower.end(), fLower.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        if (fLower.size() < 4 || fLower.compare(fLower.size() - 4, 4, ".png") != 0)
            continue;
        const std::string stem = fLower.substr(0, fLower.size() - 4);
        if (stem.size() < idLower.size() + 1)
            continue;
        if (stem.compare(stem.size() - idLower.size(), idLower.size(), idLower) != 0)
            continue;
        const char cSep = stem[stem.size() - idLower.size() - 1];
        if (cSep != '_' && cSep != '-')
            continue;
        if (idLower == "assimilator" && fLower.find("stargate") != std::string::npos)
            continue;
        if (idLower == "stalker" && fLower.find("adept") != std::string::npos)
            continue;
        if (idLower == "blink" && fLower.find("probe") != std::string::npos)
            continue;
        if (idLower == "probe" && fLower.find("mothership") != std::string::npos)
            continue;
        if (idLower == "immortal" && fLower.find("phoenix") != std::string::npos)
            continue;
        return entry.path();
    }
    return {};
}
} // namespace

void IconTextureCache::SetBaseDirectory(std::filesystem::path exeDirectory)
{
    m_exeDir = std::move(exeDirectory);
    RebuildSearchRoots();
}

void IconTextureCache::RebuildSearchRoots()
{
    m_vSearchRoots.clear();
    std::error_code ec;
    std::filesystem::path base = std::filesystem::weakly_canonical(m_exeDir, ec);
    if (ec)
        base = m_exeDir;

    auto addIfHasIcons = [this](const std::filesystem::path& root) {
        std::error_code e;
        const std::filesystem::path icons = root / "data" / "icons";
        if (!std::filesystem::is_directory(icons, e) || e)
            return;
        for (const auto& existing : m_vSearchRoots)
        {
            std::error_code eq;
            if (std::filesystem::equivalent(existing, root, eq))
                return;
        }
        m_vSearchRoots.push_back(root);
    };

    addIfHasIcons(base);

    std::filesystem::path p = base;
    std::string leaf = p.filename().string();
    if (leaf == "Release" || leaf == "Debug" || leaf == "RelWithDebInfo" || leaf == "MinSizeRel")
    {
        p = p.parent_path();
        leaf = p.filename().string();
        if (leaf == "x64" || leaf == "Win32" || leaf == "ARM64")
        {
            p = p.parent_path();
            addIfHasIcons(p);
        }
    }

    if (m_vSearchRoots.empty())
        m_vSearchRoots.push_back(base);
}

void IconTextureCache::InvalidateAll()
{
    for (auto& kv : m_loadedByKey)
    {
        if (kv.second)
            kv.second->Release();
    }
    m_loadedByKey.clear();
    if (m_pUnknownTint)
    {
        m_pUnknownTint->Release();
        m_pUnknownTint = nullptr;
    }
}

const char* IconTextureCache::RaceSubdir(BuildRace eRace)
{
    switch (eRace)
    {
    case BuildRace::Terran: return "terran";
    case BuildRace::Protoss: return "protoss";
    case BuildRace::Zerg: return "zerg";
    default: return "common";
    }
}

std::string IconTextureCache::MakeKey(BuildRace eRace, const std::string& sIconId)
{
    std::string k = RaceSubdir(eRace);
    k += '/';
    k += sIconId;
    return k;
}

IDirect3DTexture9* IconTextureCache::CreateSolidTexture(IDirect3DDevice9* pDevice, int iW, int iH, D3DCOLOR color)
{
    if (!pDevice || iW < 1 || iH < 1)
        return nullptr;
    IDirect3DTexture9* tex = nullptr;
    if (FAILED(pDevice->CreateTexture((UINT)iW, (UINT)iH, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &tex, nullptr)))
        return nullptr;
    D3DLOCKED_RECT lr{};
    if (FAILED(tex->LockRect(0, &lr, nullptr, 0)))
    {
        tex->Release();
        return nullptr;
    }
    BYTE* dst = static_cast<BYTE*>(lr.pBits);
    for (int y = 0; y < iH; ++y)
    {
        DWORD* row = reinterpret_cast<DWORD*>(dst + y * lr.Pitch);
        for (int x = 0; x < iW; ++x)
            row[x] = color;
    }
    tex->UnlockRect(0);
    return tex;
}

bool IconTextureCache::EnsureUnknownTint()
{
    if (m_pUnknownTint || !m_pDevice)
        return m_pUnknownTint != nullptr;
    m_pUnknownTint = CreateSolidTexture(m_pDevice, 4, 4, D3DCOLOR_RGBA(55, 65, 95, 255));
    return m_pUnknownTint != nullptr;
}

IDirect3DTexture9* IconTextureCache::LoadPngFile(const std::filesystem::path& path)
{
    if (!m_pDevice)
        return nullptr;

    IWICImagingFactory* factory = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_IWICImagingFactory, reinterpret_cast<void**>(&factory));
    if (FAILED(hr) || !factory)
        return nullptr;

    std::ifstream fIn(path, std::ios::binary | std::ios::ate);
    if (!fIn)
    {
        factory->Release();
        return nullptr;
    }
    const std::streamoff nLen = fIn.tellg();
    if (nLen <= 0 || nLen > 32 * 1024 * 1024)
    {
        factory->Release();
        return nullptr;
    }
    fIn.seekg(0, std::ios::beg);
    std::vector<BYTE> vFile(static_cast<size_t>(nLen));
    if (!fIn.read(reinterpret_cast<char*>(vFile.data()), nLen))
    {
        factory->Release();
        return nullptr;
    }
    const SIZE_T nBytes = static_cast<SIZE_T>(nLen);
    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, nBytes);
    if (!hMem)
    {
        factory->Release();
        return nullptr;
    }
    void* pMem = GlobalLock(hMem);
    if (!pMem)
    {
        GlobalFree(hMem);
        factory->Release();
        return nullptr;
    }
    std::memcpy(pMem, vFile.data(), nBytes);
    GlobalUnlock(hMem);
    IStream* pStream = nullptr;
    hr = CreateStreamOnHGlobal(hMem, TRUE, &pStream);
    if (FAILED(hr) || !pStream)
    {
        GlobalFree(hMem);
        factory->Release();
        return nullptr;
    }

    IWICBitmapDecoder* decoder = nullptr;
    hr = factory->CreateDecoderFromStream(pStream, nullptr, WICDecodeMetadataCacheOnLoad, &decoder);
    pStream->Release();
    if (FAILED(hr) || !decoder)
    {
        factory->Release();
        return nullptr;
    }

    IWICBitmapFrameDecode* frame = nullptr;
    hr = decoder->GetFrame(0, &frame);
    if (FAILED(hr) || !frame)
    {
        decoder->Release();
        factory->Release();
        return nullptr;
    }

    IWICFormatConverter* conv = nullptr;
    hr = factory->CreateFormatConverter(&conv);
    if (FAILED(hr) || !conv)
    {
        frame->Release();
        decoder->Release();
        factory->Release();
        return nullptr;
    }

    hr = conv->Initialize(frame, GUID_WICPixelFormat32bppBGRA, WICBitmapDitherTypeNone, nullptr, 0.f, WICBitmapPaletteTypeMedianCut);
    if (FAILED(hr))
    {
        conv->Release();
        frame->Release();
        decoder->Release();
        factory->Release();
        return nullptr;
    }

    UINT uW = 0;
    UINT uH = 0;
    conv->GetSize(&uW, &uH);
    if (uW == 0 || uH == 0)
    {
        conv->Release();
        frame->Release();
        decoder->Release();
        factory->Release();
        return nullptr;
    }

    const UINT stride = uW * 4u;
    const UINT bufSize = stride * uH;
    std::vector<BYTE> buf(bufSize);
    hr = conv->CopyPixels(nullptr, stride, bufSize, buf.data());
    conv->Release();
    frame->Release();
    decoder->Release();
    factory->Release();
    if (FAILED(hr))
        return nullptr;

    IDirect3DTexture9* tex = nullptr;
    if (FAILED(m_pDevice->CreateTexture(uW, uH, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &tex, nullptr)))
        return nullptr;

    D3DLOCKED_RECT lr{};
    if (FAILED(tex->LockRect(0, &lr, nullptr, 0)))
    {
        tex->Release();
        return nullptr;
    }
    BYTE* pDstBase = static_cast<BYTE*>(lr.pBits);
    for (UINT y = 0; y < uH; ++y)
        memcpy(pDstBase + y * lr.Pitch, buf.data() + y * stride, stride);
    tex->UnlockRect(0);
    return tex;
}

ImTextureID IconTextureCache::GetUnknownTexture()
{
    EnsureUnknownTint();
    return D3D9TexToImId(m_pUnknownTint);
}

ImTextureID IconTextureCache::GetIconTexture(BuildRace eRace, const std::string& sIconId)
{
    if (!m_pDevice || sIconId.empty())
        return GetUnknownTexture();

    std::string id = sIconId;
    std::transform(id.begin(), id.end(), id.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    if (id.size() > 4 && id.compare(id.size() - 4, 4, ".png") == 0)
        id.erase(id.size() - 4);

    const std::string key = MakeKey(eRace, id);
    const auto found = m_loadedByKey.find(key);
    if (found != m_loadedByKey.end())
        return D3D9TexToImId(found->second);

    std::error_code ec;
    IDirect3DTexture9* tex = nullptr;

    auto tryLoadFromSubdir = [&](const std::filesystem::path& iconsBase, const char* raceName) -> IDirect3DTexture9* {
        const std::filesystem::path raceDir = iconsBase / raceName;
        std::filesystem::path racePath = raceDir / (id + ".png");
        if (std::filesystem::is_regular_file(racePath, ec))
        {
            IDirect3DTexture9* pLoaded = LoadPngFile(racePath);
            if (pLoaded)
                return pLoaded;
        }
        const std::filesystem::path alt = FindIconPngInDirectoryImpl(raceDir, id);
        if (!alt.empty())
        {
            IDirect3DTexture9* pLoaded = LoadPngFile(alt);
            if (pLoaded)
                return pLoaded;
        }
        const std::filesystem::path altSuffix = FindIconPngByIdSuffixInDirectoryImpl(raceDir, id);
        if (!altSuffix.empty())
        {
            IDirect3DTexture9* pLoaded = LoadPngFile(altSuffix);
            if (pLoaded)
                return pLoaded;
        }
        return nullptr;
    };

    static const char* kRaceDirs[] = { "terran", "protoss", "zerg" };

    // Invert: repo `data/icons` (added second) overrides stale copies under x64/Release/etc.
    for (auto itRoot = m_vSearchRoots.rbegin(); itRoot != m_vSearchRoots.rend(); ++itRoot)
    {
        const std::filesystem::path& root = *itRoot;
        const std::filesystem::path iconsBase = root / "data" / "icons";

        if (eRace == BuildRace::Terran)
            tex = tryLoadFromSubdir(iconsBase, "terran");
        else if (eRace == BuildRace::Protoss)
            tex = tryLoadFromSubdir(iconsBase, "protoss");
        else if (eRace == BuildRace::Zerg)
            tex = tryLoadFromSubdir(iconsBase, "zerg");
        else
        {
            for (const char* pR : kRaceDirs)
            {
                tex = tryLoadFromSubdir(iconsBase, pR);
                if (tex)
                    break;
            }
        }

        if (!tex)
        {
            std::filesystem::path commonPath = iconsBase / "common" / (id + ".png");
            if (std::filesystem::is_regular_file(commonPath, ec))
                tex = LoadPngFile(commonPath);
            if (!tex)
            {
                std::filesystem::path pFound = FindIconPngInDirectoryImpl(iconsBase / "common", id);
                if (!pFound.empty())
                    tex = LoadPngFile(pFound);
            }
            if (!tex)
            {
                std::filesystem::path flatPath = iconsBase / (id + ".png");
                if (std::filesystem::is_regular_file(flatPath, ec))
                    tex = LoadPngFile(flatPath);
            }
            if (!tex)
            {
                std::filesystem::path pFound = FindIconPngInDirectoryImpl(iconsBase, id);
                if (!pFound.empty())
                    tex = LoadPngFile(pFound);
            }
            if (!tex)
            {
                std::filesystem::path pSuf = FindIconPngByIdSuffixInDirectoryImpl(iconsBase / "common", id);
                if (!pSuf.empty())
                    tex = LoadPngFile(pSuf);
            }
            if (!tex)
            {
                std::filesystem::path pSuf = FindIconPngByIdSuffixInDirectoryImpl(iconsBase, id);
                if (!pSuf.empty())
                    tex = LoadPngFile(pSuf);
            }
        }

        if (tex)
            break;
    }

    if (!tex)
        return GetUnknownTexture();

    m_loadedByKey[key] = tex;
    return D3D9TexToImId(tex);
}

ImTextureID IconTextureCache::GetRaceFilterTexture(BuildRace eRace)
{
    if (!m_pDevice)
        return GetUnknownTexture();
    std::string id;
    switch (eRace)
    {
    case BuildRace::Terran:
        id = "terran";
        break;
    case BuildRace::Protoss:
        id = "protoss";
        break;
    case BuildRace::Zerg:
        id = "zerg";
        break;
    default:
        return GetUnknownTexture();
    }
    const std::string key = std::string("racefilter/") + id;
    const auto found = m_loadedByKey.find(key);
    if (found != m_loadedByKey.end())
        return D3D9TexToImId(found->second);

    std::error_code ec;
    IDirect3DTexture9* tex = nullptr;
    for (auto itRoot = m_vSearchRoots.rbegin(); itRoot != m_vSearchRoots.rend(); ++itRoot)
    {
        const std::filesystem::path commonDir = (*itRoot) / "data" / "icons" / "common";
        const std::filesystem::path exact = commonDir / (id + ".png");
        if (std::filesystem::is_regular_file(exact, ec))
            tex = LoadPngFile(exact);
        if (!tex)
        {
            const std::filesystem::path pFound = FindIconPngInDirectoryImpl(commonDir, id);
            if (!pFound.empty())
                tex = LoadPngFile(pFound);
        }
        if (tex)
            break;
    }
    if (!tex)
        return GetUnknownTexture();
    m_loadedByKey[key] = tex;
    return D3D9TexToImId(tex);
}

ImTextureID IconTextureCache::GetUiIconTexture(const std::string& sName)
{
    if (!m_pDevice || sName.empty())
        return GetUnknownTexture();

    std::string id = sName;
    std::transform(id.begin(), id.end(), id.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    const std::string key = std::string("ui/") + id;
    const auto found = m_loadedByKey.find(key);
    if (found != m_loadedByKey.end())
        return D3D9TexToImId(found->second);

    std::error_code ec;
    IDirect3DTexture9* tex = nullptr;
    for (const std::filesystem::path& root : m_vSearchRoots)
    {
        const std::filesystem::path pathUi = root / "data" / "icons" / "ui" / (id + ".png");
        if (std::filesystem::is_regular_file(pathUi, ec))
        {
            tex = LoadPngFile(pathUi);
            if (tex)
                break;
        }
    }

    if (!tex)
        return GetUnknownTexture();

    m_loadedByKey[key] = tex;
    return D3D9TexToImId(tex);
}
