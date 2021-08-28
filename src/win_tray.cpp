//
// Created by Aron on 8/28/2021.
//

#include <functional>
#include "win_tray.h"

win_tray::win_tray(const handle_data& new_window) : tracked_window(new_window) {
    create_hidden_window();
    create_system_tray();
    SetProp(tracked_window.window_handle, "embedded_tray_object", this);
    namechanged_hook = SetWinEventHook(EVENT_OBJECT_NAMECHANGE, EVENT_OBJECT_NAMECHANGE, nullptr, wnd_changed_proc, 0,
                                          GetWindowThreadProcessId(tracked_window.window_handle, nullptr), WINEVENT_OUTOFCONTEXT);
}

LRESULT win_tray::wnd_proc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    auto *tray = (win_tray *) GetProp(window, "embedded_tray_object");
    switch (message) {
        case MY_TRAY_ICON_MESSAGE:
            switch (LOWORD(lParam)) {
                case WM_LBUTTONDBLCLK:
                    tray->handle_tray_dbclick();
                    break;
                case WM_RBUTTONDOWN:
                case WM_CONTEXTMENU:
                    tray->show_context_menu(window);
                    break;
            }
            break;
        case WM_DESTROY:
            Shell_NotifyIcon(NIM_DELETE, &tray->ni_data);
            break;
        default:
            // noop
            break;
    }
    return DefWindowProc(window, message, wParam, lParam);
}

void win_tray::wnd_changed_proc(HWINEVENTHOOK hWinEventHook, DWORD event, HWND hwnd, LONG idObject, LONG idChild,
                                DWORD dwEventThread, DWORD dwmsEventTime) {
    if(hwnd == nullptr) return;
    auto *tray = (win_tray *) GetProp(hwnd, "embedded_tray_object");
    if(tray == nullptr) return;
    // Check this is the window we want. Titlebar name changes result in these
    // two values (obtained by looking at some titlebar changes with the
    // Accessible Event Watcher tool in the Windows SDK)
    if (hwnd == tray->tracked_window.window_handle) {
        tray->update_title();
    }
}

void win_tray::handle_tray_dbclick() const {
    if (IsWindowVisible(tracked_window.window_handle)) {
        ShowWindow(tracked_window.window_handle, SW_HIDE);
    } else {
        ShowWindow(tracked_window.window_handle, SW_SHOW);
    }
}

void win_tray::show_context_menu(HWND context) {

    POINT pt;
    GetCursorPos(&pt);

    HMENU hMenu = CreatePopupMenu();
    InsertMenu(hMenu, 0, MF_BYPOSITION | MF_STRING, IDM_EXIT, "Exit");
    InsertMenu(hMenu, 0, MF_BYPOSITION | MF_STRING, IDM_EXIT_TRAY, "Close tray icon");

    SetForegroundWindow(context);

    int cmd = TrackPopupMenu(hMenu, TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_BOTTOMALIGN
                                    | TPM_NONOTIFY | TPM_RETURNCMD, pt.x, pt.y, 0, context, nullptr);

    PostMessage(context, WM_NULL, 0, 0);

    switch (cmd) {
        case IDM_EXIT: {
            const auto process = OpenProcess(PROCESS_TERMINATE, false, tracked_window.process_id);
            TerminateProcess(process, 1);
            CloseHandle(process);
        }

        case IDM_EXIT_TRAY:
            cleanup();
        default:
            // noop
            break;
    }
}

void win_tray::create_hidden_window() {
    HINSTANCE hInstance = GetModuleHandle(nullptr);

    static const char *class_name = "DUMMY_CLASS";
    WNDCLASSEX wx = {};
    wx.cbSize = sizeof(WNDCLASSEX);
    wx.lpfnWndProc = wnd_proc;
    wx.hInstance = hInstance;
    wx.lpszClassName = class_name;
    if (RegisterClassEx(&wx)) {
        hidden_window = CreateWindowEx(0, class_name, "Helper window", 0, 0, 0, 0, 0, HWND_MESSAGE, nullptr, nullptr,
                                       nullptr);
        SetProp(hidden_window, "embedded_tray_object", this);
    }
}

void win_tray::create_system_tray() {
    // zero the structure - note: Some Windows funtions
    // require this but I can't be bothered to remember
    // which ones do and which ones don't.


    ZeroMemory(&ni_data, sizeof(NOTIFYICONDATA));

    ni_data.cbSize = sizeof(NOTIFYICONDATA);


    // the ID number can be any UINT you choose and will
    // be used to identify your icon in later calls to
    // Shell_NotifyIcon


    ni_data.uID = TRAY_ID;


    // state which structure members are valid
    // here you can also choose the style of tooltip
    // window if any - specifying a balloon window:
    // NIF_INFO is a little more complicated


    ni_data.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP | NIF_INFO | NIF_SHOWTIP;


    // load the icon note: you should destroy the icon
    // after the call to Shell_NotifyIcon
#define GCL_HICON (-14)

    ni_data.hIcon = (HICON) GetClassLongPtr(tracked_window.window_handle, GCL_HICON);

// set the window you want to recieve event messages
    ni_data.hWnd = hidden_window;


// set the message to send
// note: the message value should be in the
// range of WM_APP through 0xBFFF


    ni_data.uCallbackMessage = MY_TRAY_ICON_MESSAGE;

    WCHAR szName[128];
    GetWindowText(tracked_window.window_handle, (LPSTR) szName, ARRAYSIZE(szName));
    lstrcpy(ni_data.szTip, (LPSTR) szName);

    Shell_NotifyIcon(NIM_ADD, &ni_data);
    Shell_NotifyIcon(NIM_SETVERSION, &ni_data); //called only when usingNIM_ADD

}

void win_tray::cleanup() {
    if(closed) {
        return;
    }
    UnhookWinEvent(namechanged_hook);
    RemoveProp(tracked_window.window_handle, "embedded_tray_object");
    DestroyWindow(hidden_window);
    Shell_NotifyIcon(NIM_DELETE, &ni_data);
    closed = true;
}

win_tray::~win_tray() {
    cleanup();
}

void win_tray::update_title() {
    WCHAR szName[128];
    GetWindowText(tracked_window.window_handle, (LPSTR) szName, ARRAYSIZE(szName));
    lstrcpy(ni_data.szTip, (LPSTR) szName);
    Shell_NotifyIcon(NIM_MODIFY, &ni_data);
}
