#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define WM_TRAYICON (WM_USER + 1)
#define WM_INJECT_KEY (WM_USER + 2)  // 钩子回调转交消息窗口，避免在钩子内重入 SendInput
#define IDM_GITHUB 1001
#define IDM_EXIT 1002
#include <windows.h>
#include <shellapi.h>

static HHOOK g_mouse_hook = nullptr;
static HWND g_msg_wnd = nullptr;
static NOTIFYICONDATAW g_nid = {};

static void send_key_event(WORD vk, bool down) {
    INPUT input{};
    input.type = INPUT_KEYBOARD;
    input.ki.wVk = vk;
    input.ki.dwFlags = down ? 0 : KEYEVENTF_KEYUP;
    SendInput(1, &input, sizeof(INPUT));
}

static LRESULT CALLBACK mouse_proc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0 && (wParam == WM_XBUTTONDOWN || wParam == WM_XBUTTONUP)) {
        auto* ms = reinterpret_cast<MSLLHOOKSTRUCT*>(lParam);
        const DWORD button = HIWORD(ms->mouseData);
        if (button == XBUTTON1 || button == XBUTTON2) {
            const WORD vk = (button == XBUTTON1) ? VK_NEXT : VK_PRIOR;
            const bool down = (wParam == WM_XBUTTONDOWN);
            // 不在钩子回调内调用 SendInput，转交消息窗口处理，使回调即时返回
            PostMessageW(g_msg_wnd, WM_INJECT_KEY, vk, down ? 1 : 0);
            return 1;
        }
    }
    return CallNextHookEx(g_mouse_hook, nCode, wParam, lParam);
}

static LRESULT CALLBACK wnd_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    if (msg == WM_INJECT_KEY) {
        send_key_event(static_cast<WORD>(wp), lp != 0);
        return 0;
    }
    if (msg == WM_TRAYICON) {
        if (lp == WM_RBUTTONUP) {
            POINT pt;
            GetCursorPos(&pt);
            HMENU hMenu = CreatePopupMenu();
            AppendMenuW(hMenu, MF_STRING, IDM_GITHUB, L"GitHub 项目主页");
            AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);
            AppendMenuW(hMenu, MF_STRING, IDM_EXIT, L"退出");
            SetForegroundWindow(hwnd);
            TrackPopupMenu(hMenu, TPM_RIGHTALIGN | TPM_BOTTOMALIGN,
                pt.x, pt.y, 0, hwnd, nullptr);
            DestroyMenu(hMenu);
        }
        return 0;
    }
    if (msg == WM_COMMAND) {
        if (LOWORD(wp) == IDM_GITHUB) {
            ShellExecuteW(nullptr, L"open",
                L"https://github.com/jark006/Xbtn2PgScroll",
                nullptr, nullptr, SW_SHOWNORMAL);
            return 0;
        }
        if (LOWORD(wp) == IDM_EXIT) {
            DestroyWindow(hwnd);
            return 0;
        }
    }
    if (msg == WM_DESTROY) {
        Shell_NotifyIconW(NIM_DELETE, &g_nid);
        if (g_mouse_hook) {
            UnhookWindowsHookEx(g_mouse_hook);
            g_mouse_hook = nullptr;
        }
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, msg, wp, lp);
}

int WINAPI wWinMain(_In_  HINSTANCE hInst, _In_opt_ HINSTANCE, _In_ LPWSTR, _In_ int nShow) {
    constexpr LPCWSTR kClassName = L"Xbtn2PgScrollClass";
    WNDCLASSEXW wc = { sizeof(wc) };
    wc.lpfnWndProc = wnd_proc;
    wc.hInstance = hInst;
    wc.hIcon = LoadIconW(hInst, L"MAIN_ICON");
    wc.lpszClassName = kClassName;
    RegisterClassExW(&wc);

    g_msg_wnd = CreateWindowExW(0, kClassName, L"", 0,
        0, 0, 0, 0, HWND_MESSAGE, nullptr, hInst, nullptr);

    g_mouse_hook = SetWindowsHookExW(WH_MOUSE_LL, mouse_proc, hInst, 0);
    if (!g_mouse_hook) {
        MessageBoxW(nullptr, L"安装鼠标钩子失败，请以管理员权限运行",
            L"Xbtn2PgScroll", MB_OK | MB_ICONERROR);
        DestroyWindow(g_msg_wnd);
        return 1;
    }

    g_nid.cbSize = sizeof(g_nid);
    g_nid.hWnd = g_msg_wnd;
    g_nid.uID = 1;
    g_nid.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE;
    g_nid.uCallbackMessage = WM_TRAYICON;
    g_nid.hIcon = LoadIconW(hInst, L"MAIN_ICON");
    lstrcpyW(g_nid.szTip, L"鼠标侧键已映射到PageUp/PageDown");
    Shell_NotifyIconW(NIM_ADD, &g_nid);

    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return static_cast<int>(msg.wParam);
}
