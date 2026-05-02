#include "PathUtils.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

std::filesystem::path GetExecutableDirectory()
{
    wchar_t buf[MAX_PATH] = {};
    const DWORD n = GetModuleFileNameW(nullptr, buf, MAX_PATH);
    if (n == 0)
        return std::filesystem::current_path();
    std::filesystem::path p(buf);
    return p.parent_path();
}
