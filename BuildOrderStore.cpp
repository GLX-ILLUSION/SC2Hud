#include "BuildOrderStore.h"
#include "BuildOrderJson.h"

#include <fstream>
#include <sstream>

namespace
{
std::string ReadAllTextUtf8(const std::filesystem::path& path, std::string& outError)
{
    outError.clear();
    std::ifstream in(path, std::ios::binary);
    if (!in)
    {
        outError = "Failed to open file.";
        return {};
    }
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

std::string SanitizeFileBaseName(std::string name)
{
    for (char& c : name)
    {
        const unsigned char uc = static_cast<unsigned char>(c);
        const bool bOk = std::isalnum(uc) != 0 || c == '_' || c == '-';
        if (!bOk)
            c = '_';
    }
    while (!name.empty() && name.front() == '_')
        name.erase(name.begin());
    while (!name.empty() && name.back() == '_')
        name.pop_back();
    if (name.empty())
        name = "custom_build";
    return name;
}
} // namespace

BuildOrderStore::BuildOrderStore(std::filesystem::path baseDirectory)
    : m_root(std::move(baseDirectory))
{
}

void BuildOrderStore::RefreshFileList()
{
    vBuilds.clear();
    std::error_code ec;
    std::filesystem::create_directories(GetPresetsPath(), ec);
    std::filesystem::create_directories(GetUserPath(), ec);
    CollectJsonFromDir(GetPresetsPath(), "presets");
    CollectJsonFromDir(GetUserPath(), "user");
}

void BuildOrderStore::CollectJsonFromDir(const std::filesystem::path& dir, const char* sRelativePrefix)
{
    std::error_code ec;
    if (!std::filesystem::exists(dir, ec))
        return;
    for (const auto& entry : std::filesystem::directory_iterator(dir, ec))
    {
        if (!entry.is_regular_file(ec))
            continue;
        if (entry.path().extension() != ".json")
            continue;
        BuildFileInfo info;
        info.sRelativePath = std::string(sRelativePrefix) + "/" + entry.path().filename().string();
        BuildOrder tmp;
        std::string err;
        if (LoadByRelativePath(info.sRelativePath, tmp, err) && !tmp.sName.empty())
            info.sDisplayName = tmp.sName + " (" + info.sRelativePath + ")";
        else
            info.sDisplayName = entry.path().filename().string();
        vBuilds.push_back(std::move(info));
    }
}

bool BuildOrderStore::LoadByRelativePath(const std::string& sRelative, BuildOrder& outOrder, std::string& outError) const
{
    std::filesystem::path path;
    if (sRelative.rfind("presets/", 0) == 0)
        path = GetPresetsPath() / std::filesystem::path(sRelative.substr(8));
    else if (sRelative.rfind("user/", 0) == 0)
        path = GetUserPath() / std::filesystem::path(sRelative.substr(5));
    else
    {
        outError = "Unknown build path prefix.";
        return false;
    }
    std::string readErr;
    const std::string json = ReadAllTextUtf8(path, readErr);
    if (!readErr.empty())
    {
        outError = readErr;
        return false;
    }
    return BuildOrderFromJsonString(json, outOrder, outError);
}

bool BuildOrderStore::SaveUserBuild(const BuildOrder& order, const std::string& sFileBaseName, std::string& outError)
{
    std::error_code ec;
    std::filesystem::create_directories(GetUserPath(), ec);
    const std::string safe = SanitizeFileBaseName(sFileBaseName);
    const std::filesystem::path path = GetUserPath() / (safe + ".json");
    std::string json;
    if (!BuildOrderToJsonString(order, json, outError))
        return false;
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out)
    {
        outError = "Failed to write user build.";
        return false;
    }
    out << json;
    RefreshFileList();
    return true;
}
