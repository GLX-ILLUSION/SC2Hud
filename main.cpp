// SC2Hud — overlay sem moldura (topmost, chroma key, sem roubar foco do jogo).
// INSERT = painel de configuracao.
// F11 = alternar modo teclado com o painel fechado (overlay pode receber texto).
// END = encerrar o programa.

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "BuildOrderStore.h"
#include "BuildOrderUi.h"
#include "MatchClock.h"
#include "PathUtils.h"
#include "UiLanguage.h"
#include "IconTextureCache.h"
#include "imgui.h"
#include "imgui_impl_dx9.h"
#include "imgui_impl_win32.h"
#include <d3d9.h>
#include <tchar.h>
#include <Windows.h>

static constexpr COLORREF kOverlayChromaKey = RGB(255, 0, 255);

enum : int
{
    kHotKeyIdFocusToggle = 1,
    kHotKeyIdConfigPanel = 2,
    kHotKeyIdExit = 3,
};

static LPDIRECT3D9           g_pD3D = nullptr;
static LPDIRECT3DDEVICE9     g_pd3dDevice = nullptr;
static bool                  g_DeviceLost = false;
static UINT                  g_ResizeWidth = 0;
static UINT                  g_ResizeHeight = 0;
static D3DPRESENT_PARAMETERS g_d3dpp = {};

static HWND  g_hWndOverlay = nullptr;
static bool  g_bOverlayPassMouseToHwnd = false;
static bool  g_bOverlayKeyboardMode = false;

static IconTextureCache g_iconCache;

bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void ResetDevice();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

static void ApplySc2HudTheme(ImGuiStyle& style)
{
    // Visual proximo ao menu Versus do SC2: paineis azul-ardosia escuros, bordas e destaques em ciano neon (#00C8FF).
    // Com LWA_COLORKEY (magenta no back buffer), mantemos alpha 255 nas tintas principais para nao misturar com o rosa.
    const auto Rgb = [](int r, int g, int b, int a) {
        return ImVec4(static_cast<float>(r) / 255.f, static_cast<float>(g) / 255.f, static_cast<float>(b) / 255.f, static_cast<float>(a) / 255.f);
    };
    style.Colors[ImGuiCol_Text] = Rgb(235, 245, 255, 255);
    style.Colors[ImGuiCol_TextDisabled] = Rgb(95, 125, 152, 255);
    style.Colors[ImGuiCol_WindowBg] = Rgb(8, 14, 28, 255);
    style.Colors[ImGuiCol_ChildBg] = Rgb(10, 18, 36, 255);
    style.Colors[ImGuiCol_PopupBg] = Rgb(10, 20, 40, 255);
    style.Colors[ImGuiCol_Border] = Rgb(0, 145, 200, 255);
    style.Colors[ImGuiCol_BorderShadow] = Rgb(0, 0, 0, 0);
    style.Colors[ImGuiCol_FrameBg] = Rgb(12, 28, 52, 255);
    style.Colors[ImGuiCol_FrameBgHovered] = Rgb(16, 42, 72, 255);
    style.Colors[ImGuiCol_FrameBgActive] = Rgb(20, 52, 88, 255);
    style.Colors[ImGuiCol_TitleBg] = Rgb(6, 12, 26, 255);
    style.Colors[ImGuiCol_TitleBgActive] = Rgb(10, 22, 48, 255);
    style.Colors[ImGuiCol_TitleBgCollapsed] = Rgb(6, 12, 26, 255);
    style.Colors[ImGuiCol_MenuBarBg] = Rgb(8, 16, 32, 255);
    style.Colors[ImGuiCol_ScrollbarBg] = Rgb(6, 12, 24, 255);
    style.Colors[ImGuiCol_ScrollbarGrab] = Rgb(0, 118, 170, 255);
    style.Colors[ImGuiCol_ScrollbarGrabHovered] = Rgb(0, 175, 225, 255);
    style.Colors[ImGuiCol_ScrollbarGrabActive] = Rgb(0, 200, 255, 255);
    style.Colors[ImGuiCol_CheckMark] = Rgb(0, 200, 255, 255);
    style.Colors[ImGuiCol_SliderGrab] = Rgb(0, 200, 255, 255);
    style.Colors[ImGuiCol_SliderGrabActive] = Rgb(130, 235, 255, 255);
    style.Colors[ImGuiCol_Button] = Rgb(12, 45, 82, 255);
    style.Colors[ImGuiCol_ButtonHovered] = Rgb(0, 105, 160, 255);
    style.Colors[ImGuiCol_ButtonActive] = Rgb(0, 75, 118, 255);
    style.Colors[ImGuiCol_Header] = Rgb(14, 38, 68, 255);
    style.Colors[ImGuiCol_HeaderHovered] = Rgb(0, 95, 145, 255);
    style.Colors[ImGuiCol_HeaderActive] = Rgb(0, 125, 180, 255);
    style.Colors[ImGuiCol_Separator] = Rgb(22, 72, 108, 255);
    style.Colors[ImGuiCol_SeparatorHovered] = Rgb(0, 175, 230, 255);
    style.Colors[ImGuiCol_SeparatorActive] = Rgb(0, 200, 255, 255);
    style.Colors[ImGuiCol_ResizeGrip] = Rgb(0, 90, 130, 255);
    style.Colors[ImGuiCol_ResizeGripHovered] = Rgb(0, 190, 240, 255);
    style.Colors[ImGuiCol_ResizeGripActive] = Rgb(0, 215, 255, 255);
    style.Colors[ImGuiCol_InputTextCursor] = Rgb(0, 200, 255, 255);
    style.Colors[ImGuiCol_TabHovered] = Rgb(18, 52, 88, 255);
    style.Colors[ImGuiCol_Tab] = Rgb(8, 18, 36, 255);
    style.Colors[ImGuiCol_TabSelected] = Rgb(12, 36, 68, 255);
    style.Colors[ImGuiCol_TabSelectedOverline] = Rgb(0, 200, 255, 255);
    style.Colors[ImGuiCol_TabDimmed] = Rgb(8, 16, 32, 255);
    style.Colors[ImGuiCol_TabDimmedSelected] = Rgb(12, 34, 62, 255);
    style.Colors[ImGuiCol_TabDimmedSelectedOverline] = Rgb(0, 195, 250, 255);
    style.Colors[ImGuiCol_TableHeaderBg] = Rgb(10, 32, 58, 255);
    style.Colors[ImGuiCol_TableBorderStrong] = Rgb(0, 130, 185, 255);
    style.Colors[ImGuiCol_TableBorderLight] = Rgb(28, 78, 112, 255);
    style.Colors[ImGuiCol_TableRowBg] = Rgb(8, 16, 32, 255);
    style.Colors[ImGuiCol_TableRowBgAlt] = Rgb(10, 22, 42, 255);
    style.Colors[ImGuiCol_TextLink] = Rgb(110, 220, 255, 255);
    style.Colors[ImGuiCol_TextSelectedBg] = Rgb(0, 72, 115, 255);
    style.Colors[ImGuiCol_TreeLines] = Rgb(0, 95, 135, 255);
    style.Colors[ImGuiCol_DragDropTarget] = Rgb(0, 200, 255, 255);
    style.Colors[ImGuiCol_DragDropTargetBg] = Rgb(0, 48, 80, 255);
    style.Colors[ImGuiCol_UnsavedMarker] = Rgb(255, 200, 100, 255);
    style.Colors[ImGuiCol_NavCursor] = Rgb(0, 200, 255, 255);
    style.Colors[ImGuiCol_NavWindowingHighlight] = Rgb(0, 200, 255, 255);
    style.Colors[ImGuiCol_NavWindowingDimBg] = Rgb(3, 10, 22, 255);
    style.Colors[ImGuiCol_ModalWindowDimBg] = Rgb(3, 10, 22, 255);
    style.Colors[ImGuiCol_PlotLines] = Rgb(80, 195, 245, 255);
    style.Colors[ImGuiCol_PlotLinesHovered] = Rgb(170, 240, 255, 255);
    style.Colors[ImGuiCol_PlotHistogram] = Rgb(0, 145, 205, 255);
    style.Colors[ImGuiCol_PlotHistogramHovered] = Rgb(0, 200, 255, 255);

    style.WindowRounding = 8.f;
    style.ChildRounding = 6.f;
    style.FrameRounding = 5.f;
    style.PopupRounding = 6.f;
    style.ScrollbarRounding = 6.f;
    style.GrabRounding = 4.f;
    style.TabRounding = 4.f;
    style.WindowBorderSize = 1.5f;
    style.ChildBorderSize = 1.f;
    style.FrameBorderSize = 0.f;
    style.TabBorderSize = 1.f;
}

static void ApplyOverlayStyles(HWND hWnd, bool bKeyboardMode)
{
    LONG_PTR ex = ::GetWindowLongPtrW(hWnd, GWL_EXSTYLE);
    ex |= WS_EX_TOPMOST | WS_EX_LAYERED;
    if (bKeyboardMode)
        ex &= ~(LONG_PTR)WS_EX_NOACTIVATE;
    else
        ex |= (LONG_PTR)WS_EX_NOACTIVATE;
    ::SetWindowLongPtrW(hWnd, GWL_EXSTYLE, ex);
    ::SetLayeredWindowAttributes(hWnd, kOverlayChromaKey, 0, LWA_COLORKEY);
    UINT uSwp = SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW | SWP_FRAMECHANGED;
    if (!bKeyboardMode)
        uSwp |= SWP_NOACTIVATE;
    ::SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, uSwp);
}

static RECT GetVirtualScreenRect()
{
    RECT r{};
    r.left = ::GetSystemMetrics(SM_XVIRTUALSCREEN);
    r.top = ::GetSystemMetrics(SM_YVIRTUALSCREEN);
    r.right = r.left + ::GetSystemMetrics(SM_CXVIRTUALSCREEN);
    r.bottom = r.top + ::GetSystemMetrics(SM_CYVIRTUALSCREEN);
    return r;
}

static void InjectOverlayMouseBeforeImGuiNewFrame(HWND hwnd)
{
    ImGuiIO& io = ImGui::GetIO();
    const bool bTransparentLastFrame = !g_bOverlayPassMouseToHwnd;

    POINT pt{};
    if (::GetCursorPos(&pt) && ::ScreenToClient(hwnd, &pt))
        io.AddMousePosEvent((float)pt.x, (float)pt.y);

    if (bTransparentLastFrame)
    {
        for (int i = 0; i < ImGuiMouseButton_COUNT; ++i)
            io.AddMouseButtonEvent(i, false);
    }
}

static void RefreshOverlayHitPassThroughFlag(bool bKeyboardEffective)
{
    ImGuiIO& io = ImGui::GetIO();
    const bool bMouseRelevant = io.WantCaptureMouse || ImGui::IsMouseDown(0) || ImGui::IsMouseDown(1) || ImGui::IsMouseDown(2) || ImGui::IsAnyItemActive() || ImGui::IsMouseDragging(ImGuiMouseButton_Left) || ImGui::IsMouseDragging(ImGuiMouseButton_Right) || ImGui::IsMouseDragging(ImGuiMouseButton_Middle);
    const bool bAnyWin = ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow | ImGuiHoveredFlags_AllowWhenBlockedByActiveItem) || ImGui::IsAnyItemHovered();
    g_bOverlayPassMouseToHwnd = bKeyboardEffective || bMouseRelevant || bAnyWin;
}

int main(int, char**)
{
    ::CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

    UiLanguageLoadFromDisk();

    ImGui_ImplWin32_EnableDpiAwareness();

    RECT desk = GetVirtualScreenRect();
    const int iW = desk.right - desk.left;
    const int iH = desk.bottom - desk.top;

    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.style = CS_CLASSDC;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = ::GetModuleHandleW(nullptr);
    wc.hCursor = ::LoadCursorW(nullptr, IDC_ARROW);
    wc.lpszClassName = L"SC2HudOverlay";
    ::RegisterClassExW(&wc);

    const DWORD dwStyle = WS_POPUP;
    const DWORD dwExStyle = WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_NOACTIVATE;
   
    //create window is detected your account get banned here XD
    g_hWndOverlay = ::CreateWindowExW(dwExStyle, wc.lpszClassName, L"SC2 Build HUD", dwStyle, desk.left, desk.top, iW, iH, HWND(nullptr), HMENU(nullptr), wc.hInstance, nullptr);

    if (!CreateDeviceD3D(g_hWndOverlay))
    {
        CleanupDeviceD3D();
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        ::CoUninitialize();
        return 1;
    }

    ::ShowWindow(g_hWndOverlay, SW_SHOW);
    ::UpdateWindow(g_hWndOverlay);
    ApplyOverlayStyles(g_hWndOverlay, false);

    if (!::RegisterHotKey(g_hWndOverlay, kHotKeyIdFocusToggle, MOD_NOREPEAT, VK_F11))
    {
        CleanupDeviceD3D();
        ::DestroyWindow(g_hWndOverlay);
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        ::CoUninitialize();
        return 1;
    }
    if (!::RegisterHotKey(g_hWndOverlay, kHotKeyIdConfigPanel, MOD_NOREPEAT, VK_INSERT))
    {
        ::UnregisterHotKey(g_hWndOverlay, kHotKeyIdFocusToggle);
        CleanupDeviceD3D();
        ::DestroyWindow(g_hWndOverlay);
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        ::CoUninitialize();
        return 1;
    }
    if (!::RegisterHotKey(g_hWndOverlay, kHotKeyIdExit, MOD_NOREPEAT, VK_END))
    {
        ::UnregisterHotKey(g_hWndOverlay, kHotKeyIdConfigPanel);
        ::UnregisterHotKey(g_hWndOverlay, kHotKeyIdFocusToggle);
        CleanupDeviceD3D();
        ::DestroyWindow(g_hWndOverlay);
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        ::CoUninitialize();
        return 1;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    io.IniFilename = nullptr;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    const HMONITOR primary = ::MonitorFromPoint(POINT{ 0, 0 }, MONITOR_DEFAULTTOPRIMARY);
    const float main_scale = ImGui_ImplWin32_GetDpiScaleForMonitor(primary);

    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(main_scale);
    style.FontScaleDpi = main_scale;
    ApplySc2HudTheme(style);

    ImGui_ImplWin32_Init(g_hWndOverlay);
    ImGui_ImplDX9_Init(g_pd3dDevice);
    g_iconCache.SetDevice(g_pd3dDevice);
    g_iconCache.SetBaseDirectory(GetExecutableDirectory());

    BuildOrderStore store(GetExecutableDirectory());
    store.RefreshFileList();
    MatchClock matchClock;
    BuildOrderHud buildHud(store, matchClock);

    buildHud.SetIconCache(&g_iconCache);

    bool done = false;
    while (!done)
    {
        MSG msg;
        while (::PeekMessageW(&msg, nullptr, 0U, 0U, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
            {
                done = true;
                break;
            }
            if (msg.message == WM_HOTKEY)
            {
                if (msg.wParam == kHotKeyIdFocusToggle)
                {
                    g_bOverlayKeyboardMode = !g_bOverlayKeyboardMode;
                    if (g_bOverlayKeyboardMode)
                    {
                        ::AllowSetForegroundWindow(ASFW_ANY);
                        ::SetForegroundWindow(g_hWndOverlay);
                    }
                }
                else if (msg.wParam == kHotKeyIdConfigPanel)
                    buildHud.ToggleConfigMenu();
                else if (msg.wParam == kHotKeyIdExit)
                    done = true;
                continue;
            }
            ::TranslateMessage(&msg);
            ::DispatchMessageW(&msg);
        }
        if (done)
            break;

        const bool bConfigMenuOpen = buildHud.IsConfigMenuOpen();
        const bool bKeyboardEffective = g_bOverlayKeyboardMode || bConfigMenuOpen;
        static bool s_bLastKeyboardEffective = false;
        if (bKeyboardEffective != s_bLastKeyboardEffective)
        {
            s_bLastKeyboardEffective = bKeyboardEffective;
            ApplyOverlayStyles(g_hWndOverlay, bKeyboardEffective);
        }
        static bool s_bPrevConfigOpen = false;
        if (bConfigMenuOpen && !s_bPrevConfigOpen)
        {
            ::AllowSetForegroundWindow(ASFW_ANY);
            ::SetForegroundWindow(g_hWndOverlay);
        }
        s_bPrevConfigOpen = bConfigMenuOpen;

        if (g_DeviceLost)
        {
            const HRESULT hr = g_pd3dDevice->TestCooperativeLevel();
            if (hr == D3DERR_DEVICELOST)
            {
                ::Sleep(10);
                continue;
            }
            if (hr == D3DERR_DEVICENOTRESET)
                ResetDevice();
            g_DeviceLost = false;
        }

        if (g_ResizeWidth != 0 && g_ResizeHeight != 0)
        {
            g_d3dpp.BackBufferWidth = g_ResizeWidth;
            g_d3dpp.BackBufferHeight = g_ResizeHeight;
            g_ResizeWidth = g_ResizeHeight = 0;
            ResetDevice();
        }

        ImGui_ImplDX9_NewFrame();
        ImGui_ImplWin32_NewFrame();
        InjectOverlayMouseBeforeImGuiNewFrame(g_hWndOverlay);
        ImGui::NewFrame();

        buildHud.RenderFrame();

        RefreshOverlayHitPassThroughFlag(bKeyboardEffective);

        ImGui::EndFrame();

        g_pd3dDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
        g_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
        g_pd3dDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
        const D3DCOLOR clear_col_dx = D3DCOLOR_XRGB(255, 0, 255);
        g_pd3dDevice->Clear(0, nullptr, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, clear_col_dx, 1.0f, 0);
        if (g_pd3dDevice->BeginScene() >= 0)
        {
            ImGui::Render();
            ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
            g_pd3dDevice->EndScene();
        }
        const HRESULT result = g_pd3dDevice->Present(nullptr, nullptr, nullptr, nullptr);
        if (result == D3DERR_DEVICELOST)
            g_DeviceLost = true;
    }

    ::UnregisterHotKey(g_hWndOverlay, kHotKeyIdExit);
    ::UnregisterHotKey(g_hWndOverlay, kHotKeyIdFocusToggle);
    ::UnregisterHotKey(g_hWndOverlay, kHotKeyIdConfigPanel);

    g_iconCache.InvalidateAll();

    ImGui_ImplDX9_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    ::DestroyWindow(g_hWndOverlay);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);

    ::CoUninitialize();
    return 0;
}

bool CreateDeviceD3D(HWND hWnd)
{
    if ((g_pD3D = Direct3DCreate9(D3D_SDK_VERSION)) == nullptr)
        return false;

    RECT rc{};
    ::GetClientRect(hWnd, &rc);
    const LONG dx = rc.right - rc.left;
    const LONG dy = rc.bottom - rc.top;
    const UINT w = (UINT)(dx > 8 ? dx : 8);
    const UINT h = (UINT)(dy > 8 ? dy : 8);

    ZeroMemory(&g_d3dpp, sizeof(g_d3dpp));
    g_d3dpp.Windowed = TRUE;
    g_d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    g_d3dpp.BackBufferWidth = w;
    g_d3dpp.BackBufferHeight = h;
    g_d3dpp.BackBufferFormat = D3DFMT_A8R8G8B8;
    g_d3dpp.EnableAutoDepthStencil = TRUE;
    g_d3dpp.AutoDepthStencilFormat = D3DFMT_D16;
    g_d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_ONE;
    if (g_pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, D3DCREATE_HARDWARE_VERTEXPROCESSING, &g_d3dpp, &g_pd3dDevice) < 0)
        return false;

    return true;
}

void CleanupDeviceD3D()
{
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
    if (g_pD3D) { g_pD3D->Release(); g_pD3D = nullptr; }
}

void ResetDevice()
{
    g_iconCache.InvalidateAll();
    ImGui_ImplDX9_InvalidateDeviceObjects();
    const HRESULT hr = g_pd3dDevice->Reset(&g_d3dpp);
    if (hr == D3DERR_INVALIDCALL)
        IM_ASSERT(0);
    ImGui_ImplDX9_CreateDeviceObjects();
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_NCHITTEST)
    {
        if (!g_bOverlayPassMouseToHwnd)
            return HTTRANSPARENT;
        return ::DefWindowProcW(hWnd, msg, wParam, lParam);
    }

    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (wParam == SIZE_MINIMIZED)
            return 0;
        g_ResizeWidth = (UINT)LOWORD(lParam);
        g_ResizeHeight = (UINT)HIWORD(lParam);
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU)
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}
