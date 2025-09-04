// UIAccessCppSample.cpp
// A minimal Win32 C++ demo that loads UIAccessDLL dynamically,
// tries to acquire UIAccess and set a 200x400 window to topmost.

#include <windows.h>
#include <tchar.h>
#include <string>
#include <sstream>

// DLL function typedefs
typedef int (__stdcall *pfnUIAccess_Initialize)();
typedef int (__stdcall *pfnUIAccess_GetPermission)();
typedef int (__stdcall *pfnUIAccess_CheckStatus)();
typedef int (__stdcall *pfnUIAccess_Cleanup)();
typedef const char* (__stdcall *pfnUIAccess_GetErrorMessage)(int errorCode);
typedef int (__stdcall *pfnUIAccess_GetLastError)();
typedef int (__stdcall *pfnUIAccess_PrepareAndRelaunchIfNeeded)();
typedef int (__stdcall *pfnUIAccess_SetTopmost)(HWND hwnd, int enable);

struct UiAccessApi {
    HMODULE hMod = nullptr;
    pfnUIAccess_Initialize Initialize = nullptr;
    pfnUIAccess_GetPermission GetPermission = nullptr;
    pfnUIAccess_CheckStatus CheckStatus = nullptr;
    pfnUIAccess_Cleanup Cleanup = nullptr;
    pfnUIAccess_GetErrorMessage GetErrorMessage = nullptr;
    pfnUIAccess_GetLastError GetLastError = nullptr;
    pfnUIAccess_PrepareAndRelaunchIfNeeded PrepareAndRelaunchIfNeeded = nullptr;
    pfnUIAccess_SetTopmost SetTopmost = nullptr;

    bool Load() {
        hMod = ::LoadLibraryW(L"UIAccessDLL.dll");
        if (!hMod) return false;
        Initialize = (pfnUIAccess_Initialize)::GetProcAddress(hMod, "UIAccess_Initialize");
        GetPermission = (pfnUIAccess_GetPermission)::GetProcAddress(hMod, "UIAccess_GetPermission");
        CheckStatus = (pfnUIAccess_CheckStatus)::GetProcAddress(hMod, "UIAccess_CheckStatus");
        Cleanup = (pfnUIAccess_Cleanup)::GetProcAddress(hMod, "UIAccess_Cleanup");
        GetErrorMessage = (pfnUIAccess_GetErrorMessage)::GetProcAddress(hMod, "UIAccess_GetErrorMessage");
        GetLastError = (pfnUIAccess_GetLastError)::GetProcAddress(hMod, "UIAccess_GetLastError");
        PrepareAndRelaunchIfNeeded = (pfnUIAccess_PrepareAndRelaunchIfNeeded)::GetProcAddress(hMod, "UIAccess_PrepareAndRelaunchIfNeeded");
        SetTopmost = (pfnUIAccess_SetTopmost)::GetProcAddress(hMod, "UIAccess_SetTopmost");
        if (!Initialize || !GetPermission || !CheckStatus || !Cleanup || !GetErrorMessage || !GetLastError || !PrepareAndRelaunchIfNeeded || !SetTopmost) return false;
        return true;
    }
    void Unload() {
        if (hMod) { ::FreeLibrary(hMod); hMod = nullptr; }
    }
};

static const wchar_t kWndClassName[] = L"UIAccessCppSampleWndClass";
static UiAccessApi g_api;
static HWND g_hButton = nullptr;
static HWND g_hChkTop = nullptr;

static std::wstring Utf8ToWide(const char* s) {
    if (!s) return L"";
    int wlen = MultiByteToWideChar(CP_UTF8, 0, s, -1, nullptr, 0);
    if (wlen <= 0) return L"";
    std::wstring ws;
    ws.resize(wlen - 1);
    MultiByteToWideChar(CP_UTF8, 0, s, -1, &ws[0], wlen);
    return ws;
}

static void ShowStatus(HWND hWnd, const wchar_t* title, const std::wstring& detail) {
    ::MessageBoxW(hWnd, detail.c_str(), title, MB_OK | MB_ICONINFORMATION);
}

static void TryAcquireAndTopmost(HWND hWnd) {
    if (!g_api.hMod) {
        ShowStatus(hWnd, L"UIAccess", L"UIAccessDLL.dll not found. Place it next to the EXE or add to PATH.");
        return;
    }
    int rc = g_api.Initialize();
    if (rc != 0) {
        std::wstring msg = L"Initialize failed: ";
        msg += Utf8ToWide(g_api.GetErrorMessage(rc));
        ShowStatus(hWnd, L"UIAccess", msg);
        return;
    }

    int status = g_api.CheckStatus();
    bool already = (status == 1);
    std::wstringstream ss0;
    ss0 << L"Initial UIAccess status: " << (already ? L"Enabled" : L"Disabled");
    ShowStatus(hWnd, L"UIAccess", ss0.str());

    if (!already) {
        int getRc = g_api.GetPermission();
        std::wstringstream ss;
        ss << L"GetPermission result: " << getRc << L" (" << Utf8ToWide(g_api.GetErrorMessage(getRc)) << L")";
        if (getRc != 0) {
            ss << L"\nNote: Run as Administrator is required.";
            ShowStatus(hWnd, L"UIAccess", ss.str());
        } else {
            ShowStatus(hWnd, L"UIAccess", ss.str());
        }
    }

    // Regardless of status, try to set TOPMOST for demo purpose
    RECT rcWnd{};
    ::GetWindowRect(hWnd, &rcWnd);
    BOOL ok = ::SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0,
                             SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    std::wstringstream ss2;
    ss2 << L"SetWindowPos(HWND_TOPMOST) => " << (ok ? L"OK" : L"FAILED");
    if (!ok) {
        DWORD gle = GetLastError();
        ss2 << L", GetLastError=" << gle;
    }
    ss2 << L"\nHint:\n"
           L"  - True UIAccess band above System Tools typically requires a relaunch under a UIAccess-enabled token.\n"
           L"  - This DLL attempts to prepare UIAccess; if CheckStatus stays Disabled, relaunch may be needed.";
    ShowStatus(hWnd, L"UIAccess", ss2.str());

    g_api.Cleanup();
}

static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        g_hButton = CreateWindowW(L"BUTTON", L"Get UIAccess && Topmost",
                                  WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                                  20, 20, 160, 30, hWnd, (HMENU)1001,
                                  ((LPCREATESTRUCT)lParam)->hInstance, nullptr);
        g_hChkTop = CreateWindowW(L"BUTTON", L"Always on top",
                                  WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                                  20, 60, 160, 20, hWnd, (HMENU)1002,
                                  ((LPCREATESTRUCT)lParam)->hInstance, nullptr);
        // 默认勾选，根据复选框状态立即应用置顶
        SendMessageW(g_hChkTop, BM_SETCHECK, BST_CHECKED, 0);
        if (g_api.hMod) {
            g_api.SetTopmost(hWnd, 1);
        }
        break;
    }
    case WM_COMMAND: {
        switch (LOWORD(wParam)) {
        case 1001:
            TryAcquireAndTopmost(hWnd);
            break;
        case 1002:
            if (g_api.hMod) {
                LRESULT chk = SendMessageW(g_hChkTop, BM_GETCHECK, 0, 0);
                g_api.SetTopmost(hWnd, (chk == BST_CHECKED) ? 1 : 0);
            }
            break;
        }
        break;
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

int APIENTRY wWinMain(HINSTANCE hInst, HINSTANCE, LPWSTR, int nCmdShow) {
    // Load DLL upfront (optional)
    g_api.Load();
    if (g_api.hMod) {
        // 初始化并尝试无感获取（若需会自重启）
        if (g_api.Initialize) g_api.Initialize();
        if (g_api.PrepareAndRelaunchIfNeeded) g_api.PrepareAndRelaunchIfNeeded();
    }

    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.hInstance = hInst;
    wc.lpfnWndProc = WndProc;
    wc.lpszClassName = kWndClassName;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    if (!RegisterClassExW(&wc)) return 0;

    // Create 200 x 400 window centered-ish
    int width = 200, height = 400;
    RECT r{ 0,0,width,height };
    AdjustWindowRectEx(&r, WS_OVERLAPPEDWINDOW, FALSE, 0);
    int w = r.right - r.left;
    int h = r.bottom - r.top;

    HWND hWnd = CreateWindowExW(0, kWndClassName, L"UIAccess C++ Sample (200x400)",
                                WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
                                w, h, nullptr, nullptr, hInst, nullptr);
    if (!hWnd) return 0;

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    // Message loop
    MSG m;
    while (GetMessageW(&m, nullptr, 0, 0) > 0) {
        TranslateMessage(&m);
        DispatchMessageW(&m);
    }

    g_api.Unload();
    return (int)m.wParam;
}