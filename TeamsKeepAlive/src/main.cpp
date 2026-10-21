// TeamsKeepAlive
// Keeps Microsoft Teams presence "Available" (green) by preventing the
// system idle timer from expiring.
//
// How it works
//  - Teams switches to "Away" when the OS reports no user input for ~5 min
//    (it reads the system idle time, i.e. GetLastInputInfo).
//  - SetThreadExecutionState alone only prevents sleep / display off; it does
//    NOT reset the idle timer, so Teams still goes Away (PowerToys Awake case).
//  - Therefore we inject an F15 key press with SendInput when the user has
//    been idle for a while. F15 does not exist on regular keyboards and is not
//    bound by applications, so it has no visible side effect (Caffeine style).

#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif

#include <windows.h>
#include <shellapi.h>

#include <cstdio>
#include <cwchar>

namespace
{
constexpr wchar_t kAppName[]    = L"TeamsKeepAlive";
constexpr wchar_t kMutexName[]  = L"Local\\TeamsKeepAlive_SingleInstance";
constexpr UINT    kTrayMsg      = WM_APP + 1;
constexpr UINT    kTrayId       = 1;
constexpr UINT_PTR kTimerId     = 1;

// Check every 30 s; inject F15 once the user has been idle for 60 s or more.
// Teams' Away threshold is ~5 min, so this leaves a wide margin.
#ifndef TKA_TIMER_INTERVAL_MS
#define TKA_TIMER_INTERVAL_MS (30 * 1000)
#endif
#ifndef TKA_IDLE_THRESHOLD_MS
#define TKA_IDLE_THRESHOLD_MS (60 * 1000)
#endif
constexpr UINT  kTimerIntervalMs = TKA_TIMER_INTERVAL_MS;
constexpr DWORD kIdleThresholdMs = TKA_IDLE_THRESHOLD_MS;

enum MenuId : UINT
{
    kMenuToggle = 100,
    kMenuExit,
};

HICON g_iconActive = nullptr;
HICON g_iconPaused = nullptr;
bool  g_active     = true;
UINT  g_taskbarCreatedMsg = 0;
DWORD g_lastSentTick = 0;

DWORD GetIdleMs()
{
    LASTINPUTINFO lii = {};
    lii.cbSize = sizeof(lii);
    if (!GetLastInputInfo(&lii))
        return 0;
    return GetTickCount() - lii.dwTime;
}

bool SendF15()
{
    INPUT inputs[2] = {};
    inputs[0].type       = INPUT_KEYBOARD;
    inputs[0].ki.wVk     = VK_F15;
    inputs[1].type       = INPUT_KEYBOARD;
    inputs[1].ki.wVk     = VK_F15;
    inputs[1].ki.dwFlags = KEYEVENTF_KEYUP;
    return SendInput(2, inputs, sizeof(INPUT)) == 2;
}

void SetKeepAwake(bool on)
{
    // Also keep the system and display awake so that sleep / screen-off
    // (which would lock the session and make Teams go Away) does not happen.
    if (on)
        SetThreadExecutionState(ES_CONTINUOUS | ES_SYSTEM_REQUIRED | ES_DISPLAY_REQUIRED);
    else
        SetThreadExecutionState(ES_CONTINUOUS);
}

// Draws a filled circle icon so the exe needs no external resource file.
HICON CreateDotIcon(COLORREF color)
{
    const int size = GetSystemMetrics(SM_CXSMICON);
    HDC screen = GetDC(nullptr);
    HDC dc = CreateCompatibleDC(screen);
    HBITMAP color_bmp = CreateCompatibleBitmap(screen, size, size);
    HBITMAP mask_bmp  = CreateBitmap(size, size, 1, 1, nullptr);

    // Mask: white = transparent, black = opaque.
    HGDIOBJ old = SelectObject(dc, mask_bmp);
    PatBlt(dc, 0, 0, size, size, WHITENESS);
    SelectObject(dc, GetStockObject(BLACK_BRUSH));
    SelectObject(dc, GetStockObject(BLACK_PEN));
    Ellipse(dc, 1, 1, size - 1, size - 1);

    // Color: dot on black background (black is XOR-neutral where masked).
    SelectObject(dc, color_bmp);
    PatBlt(dc, 0, 0, size, size, BLACKNESS);
    HBRUSH brush = CreateSolidBrush(color);
    HPEN pen = CreatePen(PS_SOLID, 1, RGB(255, 255, 255));
    SelectObject(dc, brush);
    SelectObject(dc, pen);
    Ellipse(dc, 1, 1, size - 1, size - 1);

    SelectObject(dc, old);
    DeleteObject(brush);
    DeleteObject(pen);

    ICONINFO ii = {};
    ii.fIcon    = TRUE;
    ii.hbmMask  = mask_bmp;
    ii.hbmColor = color_bmp;
    HICON icon = CreateIconIndirect(&ii);

    DeleteObject(color_bmp);
    DeleteObject(mask_bmp);
    DeleteDC(dc);
    ReleaseDC(nullptr, screen);
    return icon;
}

void FillTrayData(NOTIFYICONDATAW& nid, HWND hwnd)
{
    nid = {};
    nid.cbSize = sizeof(nid);
    nid.hWnd   = hwnd;
    nid.uID    = kTrayId;
    nid.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE;
    nid.uCallbackMessage = kTrayMsg;
    nid.hIcon  = g_active ? g_iconActive : g_iconPaused;
    swprintf(nid.szTip, sizeof(nid.szTip) / sizeof(nid.szTip[0]), L"%ls - %ls",
             kAppName, g_active ? L"동작 중 (초록 유지)" : L"일시정지");
}

void AddTrayIcon(HWND hwnd)
{
    NOTIFYICONDATAW nid;
    FillTrayData(nid, hwnd);
    Shell_NotifyIconW(NIM_ADD, &nid);
}

void UpdateTrayIcon(HWND hwnd)
{
    NOTIFYICONDATAW nid;
    FillTrayData(nid, hwnd);
    Shell_NotifyIconW(NIM_MODIFY, &nid);
}

void RemoveTrayIcon(HWND hwnd)
{
    NOTIFYICONDATAW nid;
    FillTrayData(nid, hwnd);
    Shell_NotifyIconW(NIM_DELETE, &nid);
}

void SetActive(HWND hwnd, bool active)
{
    g_active = active;
    SetKeepAwake(active);
    UpdateTrayIcon(hwnd);
}

void OnTimer()
{
    if (!g_active)
        return;
    if (GetIdleMs() >= kIdleThresholdMs && SendF15())
        g_lastSentTick = GetTickCount();
}

void ShowTrayMenu(HWND hwnd)
{
    wchar_t status[128];
    if (g_lastSentTick == 0)
        swprintf(status, 128, L"마지막 입력 신호: 없음");
    else
        swprintf(status, 128, L"마지막 입력 신호: %lu초 전",
                 (unsigned long)((GetTickCount() - g_lastSentTick) / 1000));

    HMENU menu = CreatePopupMenu();
    AppendMenuW(menu, MF_STRING | MF_GRAYED, 0, status);
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING, kMenuToggle, g_active ? L"일시정지" : L"재개");
    AppendMenuW(menu, MF_STRING, kMenuExit, L"종료");

    POINT pt;
    GetCursorPos(&pt);
    // Required so the menu closes when clicking elsewhere.
    SetForegroundWindow(hwnd);
    TrackPopupMenu(menu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hwnd, nullptr);
    PostMessageW(hwnd, WM_NULL, 0, 0);
    DestroyMenu(menu);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg)
    {
    case WM_CREATE:
        AddTrayIcon(hwnd);
        SetKeepAwake(true);
        SetTimer(hwnd, kTimerId, kTimerIntervalMs, nullptr);
        return 0;

    case WM_TIMER:
        if (wp == kTimerId)
            OnTimer();
        return 0;

    case kTrayMsg:
        switch (LOWORD(lp))
        {
        case WM_LBUTTONDBLCLK:
            SetActive(hwnd, !g_active);
            break;
        case WM_RBUTTONUP:
        case WM_CONTEXTMENU:
            ShowTrayMenu(hwnd);
            break;
        }
        return 0;

    case WM_COMMAND:
        switch (LOWORD(wp))
        {
        case kMenuToggle:
            SetActive(hwnd, !g_active);
            break;
        case kMenuExit:
            DestroyWindow(hwnd);
            break;
        }
        return 0;

    case WM_DESTROY:
        KillTimer(hwnd, kTimerId);
        SetKeepAwake(false);
        RemoveTrayIcon(hwnd);
        PostQuitMessage(0);
        return 0;
    }

    // Explorer restarted: tray icons are lost and must be re-added.
    if (msg == g_taskbarCreatedMsg && g_taskbarCreatedMsg != 0)
    {
        AddTrayIcon(hwnd);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

// "--selftest": verifies that SendInput(F15) resets the system idle timer,
// which is exactly what Teams looks at. Prints the result and exits.
int RunSelfTest()
{
    // GUI subsystem exe: keep stdout if it was redirected, otherwise attach
    // to the launching console (cmd / PowerShell) so the result is visible.
    HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
    if ((out == nullptr || out == INVALID_HANDLE_VALUE) &&
        (AttachConsole(ATTACH_PARENT_PROCESS) || AllocConsole()))
    {
        freopen("CONOUT$", "w", stdout);
    }
    std::printf("[selftest] waiting 3 s without input...\n");
    Sleep(3000);
    DWORD before = GetIdleMs();
    bool sent = SendF15();
    Sleep(100);
    DWORD after = GetIdleMs();
    bool ok = sent && after < before && after < 1000;
    std::printf("[selftest] idle before = %lu ms, SendInput = %s, idle after = %lu ms\n",
                (unsigned long)before, sent ? "ok" : "fail", (unsigned long)after);
    std::printf("[selftest] %s\n", ok ? "PASS: idle timer reset by F15" : "FAIL");
    std::fflush(stdout);
    return ok ? 0 : 1;
}
} // namespace

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR cmdline, int)
{
    if (cmdline && wcsstr(cmdline, L"--selftest"))
        return RunSelfTest();

    HANDLE mutex = CreateMutexW(nullptr, TRUE, kMutexName);
    if (GetLastError() == ERROR_ALREADY_EXISTS)
    {
        MessageBoxW(nullptr, L"TeamsKeepAlive가 이미 실행 중입니다.\n트레이 아이콘을 확인하세요.",
                    kAppName, MB_OK | MB_ICONINFORMATION);
        return 0;
    }

    g_taskbarCreatedMsg = RegisterWindowMessageW(L"TaskbarCreated");
    g_iconActive = CreateDotIcon(RGB(0x6B, 0xB7, 0x00));  // Teams "Available" green
    g_iconPaused = CreateDotIcon(RGB(0x9E, 0x9E, 0x9E));

    WNDCLASSW wc = {};
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = instance;
    wc.lpszClassName = kAppName;
    RegisterClassW(&wc);

    // Hidden top-level window (not HWND_MESSAGE so it still receives the
    // TaskbarCreated broadcast); the tray icon is the only UI.
    HWND hwnd = CreateWindowW(kAppName, kAppName, 0, 0, 0, 0, 0,
                              nullptr, nullptr, instance, nullptr);
    if (!hwnd)
        return 1;

    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    DestroyIcon(g_iconActive);
    DestroyIcon(g_iconPaused);
    if (mutex)
    {
        ReleaseMutex(mutex);
        CloseHandle(mutex);
    }
    return 0;
}
