#pragma once

#include "BuildOrderTypes.h"

#include <filesystem>
#include <string>
#include <vector>

struct BuildFileInfo
{
    std::string sRelativePath;
    std::string sDisplayName;
};

class BuildOrderStore
{
public:
    explicit BuildOrderStore(std::filesystem::path baseDirectory);

    void RefreshFileList();

    const std::vector<BuildFileInfo>& GetKnownBuilds() const { return vBuilds; }

    bool LoadByRelativePath(const std::string& sRelative, BuildOrder& outOrder, std::string& outError) const;

    bool SaveUserBuild(const BuildOrder& order, const std::string& sFileBaseName, std::string& outError);

    std::filesystem::path GetPresetsPath() const { return m_root / "data" / "builds" / "presets"; }
    std::filesystem::path GetUserPath() const { return m_root / "data" / "builds" / "user"; }

private:
    std::filesystem::path m_root;
    std::vector<BuildFileInfo> vBuilds;

    void CollectJsonFromDir(const std::filesystem::path& dir, const char* sRelativePrefix);
};
