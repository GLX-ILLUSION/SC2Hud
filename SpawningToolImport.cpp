#include "SpawningToolImport.h"

#include "BuildOrderPasteParser.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#include <winhttp.h>

#pragma comment(lib, "winhttp.lib")

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <sstream>
#include <vector>
#include <cwctype>

namespace
{
bool IsAsciiSpace(unsigned char c)
{
    return c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '\f' || c == '\v';
}

void TrimInPlace(std::string& s)
{
    while (!s.empty() && IsAsciiSpace(static_cast<unsigned char>(s.back())))
        s.pop_back();
    size_t i0 = 0;
    while (i0 < s.size() && IsAsciiSpace(static_cast<unsigned char>(s[i0])))
        ++i0;
    if (i0 > 0)
        s.erase(0, i0);
}

std::wstring Utf8ToWide(const std::string& sUtf8)
{
    if (sUtf8.empty())
        return {};
    const int n = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, sUtf8.c_str(), static_cast<int>(sUtf8.size()), nullptr, 0);
    if (n <= 0)
        return {};
    std::wstring w;
    w.resize(static_cast<size_t>(n));
    if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, sUtf8.c_str(), static_cast<int>(sUtf8.size()), w.data(), n) <= 0)
        return {};
    return w;
}

std::string WideToUtf8(const std::wstring& w)
{
    if (w.empty())
        return {};
    const int n = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), static_cast<int>(w.size()), nullptr, 0, nullptr, nullptr);
    if (n <= 0)
        return {};
    std::string s;
    s.resize(static_cast<size_t>(n));
    if (WideCharToMultiByte(CP_UTF8, 0, w.c_str(), static_cast<int>(w.size()), s.data(), n, nullptr, nullptr) <= 0)
        return {};
    return s;
}

bool IsAllowedSpawningToolHost(const std::wstring& hostLower)
{
    static const wchar_t kSuffix[] = L".spawningtool.com";
    constexpr size_t nSuffix = sizeof(kSuffix) / sizeof(kSuffix[0]) - 1;
    if (hostLower.size() < nSuffix)
        return false;
    return hostLower.compare(hostLower.size() - nSuffix, nSuffix, kSuffix) == 0;
}

bool UrlLooksLikeSpawningToolBuild(const std::string& sUrl)
{
    std::string u = sUrl;
    TrimInPlace(u);
    if (u.size() < 20)
        return false;
    std::string low;
    low.reserve(u.size());
    for (unsigned char c : u)
        low.push_back(static_cast<char>(std::tolower(c)));
    if (low.find("spawningtool.com") == std::string::npos)
        return false;
    if (low.find("/build/") == std::string::npos)
        return false;
    return true;
}

bool HttpsGetUtf8(const std::wstring& wUrl, std::string& outUtf8Body, std::wstring& outWinErr)
{
    outUtf8Body.clear();
    outWinErr.clear();

    URL_COMPONENTSW comp{};
    comp.dwStructSize = sizeof(comp);
    comp.dwSchemeLength = static_cast<DWORD>(-1);
    comp.dwHostNameLength = static_cast<DWORD>(-1);
    comp.dwUrlPathLength = static_cast<DWORD>(-1);
    comp.dwExtraInfoLength = static_cast<DWORD>(-1);

    if (!WinHttpCrackUrl(wUrl.c_str(), static_cast<DWORD>(wUrl.size()), 0, &comp))
    {
        outWinErr = L"URL invalida.";
        return false;
    }
    if (!comp.lpszHostName || comp.dwHostNameLength == 0)
    {
        outWinErr = L"Sem host na URL.";
        return false;
    }
    std::wstring host(comp.lpszHostName, comp.lpszHostName + comp.dwHostNameLength);
    std::wstring path(comp.lpszUrlPath, comp.lpszUrlPath + comp.dwUrlPathLength);
    if (comp.lpszExtraInfo && comp.dwExtraInfoLength > 0)
        path.append(comp.lpszExtraInfo, comp.lpszExtraInfo + comp.dwExtraInfoLength);

    std::wstring hostLower = host;
    for (wchar_t& ch : hostLower)
        ch = static_cast<wchar_t>(std::towlower(ch));
    if (!IsAllowedSpawningToolHost(hostLower))
    {
        outWinErr = L"Apenas URLs em spawningtool.com (build orders).";
        return false;
    }

    const bool bHttps = comp.nScheme == INTERNET_SCHEME_HTTPS;
    const INTERNET_PORT port = bHttps ? INTERNET_DEFAULT_HTTPS_PORT : INTERNET_DEFAULT_HTTP_PORT;
    const DWORD flags = bHttps ? WINHTTP_FLAG_SECURE : 0;

    HINTERNET hSession = WinHttpOpen(L"SC2Hud/1.0 (import build)", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession)
    {
        outWinErr = L"WinHttpOpen falhou.";
        return false;
    }
    HINTERNET hConnect = WinHttpConnect(hSession, host.c_str(), port, 0);
    if (!hConnect)
    {
        WinHttpCloseHandle(hSession);
        outWinErr = L"WinHttpConnect falhou.";
        return false;
    }
    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", path.empty() ? L"/" : path.c_str(), nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
    if (!hRequest)
    {
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        outWinErr = L"WinHttpOpenRequest falhou.";
        return false;
    }

    const wchar_t* extraHeaders = L"Accept: text/html,application/xhtml+xml\r\nAccept-Language: en-US,en;q=0.9\r\n";
    BOOL bOk = WinHttpSendRequest(hRequest, extraHeaders, static_cast<DWORD>(-1), WINHTTP_NO_REQUEST_DATA, 0, 0, 0);
    if (!bOk)
    {
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        outWinErr = L"WinHttpSendRequest falhou.";
        return false;
    }
    bOk = WinHttpReceiveResponse(hRequest, nullptr);
    if (!bOk)
    {
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        outWinErr = L"WinHttpReceiveResponse falhou.";
        return false;
    }

    DWORD dwStatus = 0;
    DWORD dwStatusSize = sizeof(dwStatus);
    if (!WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, &dwStatus, &dwStatusSize, WINHTTP_NO_HEADER_INDEX))
    {
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        outWinErr = L"Resposta HTTP sem status.";
        return false;
    }
    if (dwStatus < 200 || dwStatus >= 300)
    {
        wchar_t wbuf[64];
        _snwprintf_s(wbuf, _TRUNCATE, L"HTTP %lu.", static_cast<unsigned long>(dwStatus));
        outWinErr = wbuf;
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return false;
    }

    std::vector<char> vAcc;
    for (;;)
    {
        DWORD dwAvail = 0;
        if (!WinHttpQueryDataAvailable(hRequest, &dwAvail))
            break;
        if (dwAvail == 0)
            break;
        const size_t iOld = vAcc.size();
        if (iOld + dwAvail > 8 * 1024 * 1024)
        {
            outWinErr = L"Pagina demasiado grande.";
            WinHttpCloseHandle(hRequest);
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
            return false;
        }
        vAcc.resize(iOld + dwAvail);
        DWORD dwRead = 0;
        if (!WinHttpReadData(hRequest, vAcc.data() + iOld, dwAvail, &dwRead))
            break;
        vAcc.resize(iOld + dwRead);
    }
    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);

    if (vAcc.empty())
    {
        outWinErr = L"Corpo vazio.";
        return false;
    }

    int nLen = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, vAcc.data(), static_cast<int>(vAcc.size()), nullptr, 0);
    std::wstring wHtml;
    if (nLen > 0)
    {
        wHtml.resize(static_cast<size_t>(nLen));
        MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, vAcc.data(), static_cast<int>(vAcc.size()), wHtml.data(), nLen);
    }
    else
    {
        nLen = MultiByteToWideChar(CP_ACP, 0, vAcc.data(), static_cast<int>(vAcc.size()), nullptr, 0);
        if (nLen <= 0)
        {
            outWinErr = L"Codificacao da pagina desconhecida.";
            return false;
        }
        wHtml.resize(static_cast<size_t>(nLen));
        MultiByteToWideChar(CP_ACP, 0, vAcc.data(), static_cast<int>(vAcc.size()), wHtml.data(), nLen);
    }

    outUtf8Body = WideToUtf8(wHtml);
    return !outUtf8Body.empty();
}

void StripTagsInPlace(std::string& s)
{
    std::string out;
    out.reserve(s.size());
    bool bInTag = false;
    for (unsigned char c : s)
    {
        if (c == '<')
            bInTag = true;
        else if (c == '>')
            bInTag = false;
        else if (!bInTag)
            out.push_back(static_cast<char>(c));
    }
    s.swap(out);
}

void DecodeBasicEntities(std::string& s)
{
    for (;;)
    {
        const size_t iAmp = s.find('&');
        if (iAmp == std::string::npos)
            break;
        size_t iSemi = s.find(';', iAmp + 1);
        if (iSemi == std::string::npos)
            break;
        const std::string ent = s.substr(iAmp + 1, iSemi - iAmp - 1);
        std::string rep;
        if (ent == "amp")
            rep = "&";
        else if (ent == "lt")
            rep = "<";
        else if (ent == "gt")
            rep = ">";
        else if (ent == "quot")
            rep = "\"";
        else if (ent == "nbsp")
            rep = " ";
        else
            break;
        s.replace(iAmp, iSemi - iAmp + 1, rep);
    }
}

bool IsAllDigits(const std::string& s)
{
    if (s.empty())
        return false;
    for (unsigned char c : s)
    {
        if (!std::isdigit(c))
            return false;
    }
    return true;
}

std::string ExtractH1Plain(const std::string& html)
{
    const std::string tagOpen = "<h1";
    size_t iH = html.find(tagOpen);
    if (iH == std::string::npos)
        return {};
    const size_t iGt = html.find('>', iH + tagOpen.size());
    if (iGt == std::string::npos)
        return {};
    const size_t iEnd = html.find("</h1>", iGt + 1);
    if (iEnd == std::string::npos)
        return {};
    std::string inner = html.substr(iGt + 1, iEnd - iGt - 1);
    StripTagsInPlace(inner);
    DecodeBasicEntities(inner);
    TrimInPlace(inner);
    return inner;
}

bool BuildPasteFromSpawningToolHtml(const std::string& html, std::string& outPaste, std::string& outErr)
{
    outPaste.clear();
    outErr.clear();

    const std::string sTitle = ExtractH1Plain(html);
    size_t iPos = 0;
    std::vector<std::string> vLines;
    while (iPos < html.size())
    {
        const size_t iTr = html.find("<tr", iPos);
        if (iTr == std::string::npos)
            break;
        const size_t iTrEnd = html.find("</tr>", iTr);
        if (iTrEnd == std::string::npos)
            break;
        std::string row = html.substr(iTr, iTrEnd - iTr + 5);
        iPos = iTrEnd + 5;

        if (row.find("<th") != std::string::npos)
            continue;

        std::vector<std::string> vCells;
        size_t cPos = 0;
        while (cPos < row.size())
        {
            const size_t iTd = row.find("<td", cPos);
            if (iTd == std::string::npos)
                break;
            const size_t iGt = row.find('>', iTd + 3);
            if (iGt == std::string::npos)
                break;
            const size_t iClose = row.find("</td>", iGt + 1);
            if (iClose == std::string::npos)
                break;
            std::string cell = row.substr(iGt + 1, iClose - iGt - 1);
            StripTagsInPlace(cell);
            DecodeBasicEntities(cell);
            TrimInPlace(cell);
            vCells.push_back(std::move(cell));
            cPos = iClose + 5;
        }
        if (vCells.size() < 3)
            continue;
        if (!IsAllDigits(vCells[0]))
            continue;

        std::ostringstream line;
        line << vCells[0] << "  " << vCells[1] << "  " << vCells[2];
        if (vCells.size() >= 4 && !vCells[3].empty())
            line << "  " << vCells[3];
        vLines.push_back(line.str());
    }

    if (vLines.empty())
    {
        outErr = "Nao foi encontrada tabela de build order no HTML. Copia manual ou verifica a pagina.";
        return false;
    }

    std::ostringstream paste;
    if (!sTitle.empty())
        paste << sTitle << "\n\n";
    for (const std::string& ln : vLines)
        paste << ln << "\n";
    outPaste = paste.str();
    return true;
}
} // namespace

bool ImportBuildOrderFromSpawningToolUrl(const std::string& sUrlUtf8, BuildOrder& outOrder, std::string& outError)
{
    outError.clear();
    outOrder.Clear();

    std::string sUrl = sUrlUtf8;
    TrimInPlace(sUrl);
    if (sUrl.empty())
    {
        outError = "URL vazia.";
        return false;
    }
    if (sUrl.find("://") == std::string::npos)
        sUrl.insert(0, "https://");
    if (!UrlLooksLikeSpawningToolBuild(sUrl))
    {
        outError = "Use um link do Spawning Tool, ex.: https://lotv.spawningtool.com/build/123456/";
        return false;
    }

    std::wstring wUrl = Utf8ToWide(sUrl);
    if (wUrl.empty())
    {
        outError = "URL invalida (UTF-8).";
        return false;
    }

    std::string html;
    std::wstring wErr;
    if (!HttpsGetUtf8(wUrl, html, wErr))
    {
        if (wErr.empty())
            outError = "Falha ao descarregar a pagina.";
        else
            outError = WideToUtf8(wErr);
        return false;
    }

    std::string paste;
    if (!BuildPasteFromSpawningToolHtml(html, paste, outError))
        return false;

    if (!ParseWebsiteBuildOrderPaste(paste, outOrder, outError))
        return false;

    return true;
}
