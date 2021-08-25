#include <windows.h>
#include <tchar.h>
#include <psapi.h>
#include <string>
#include <iostream>

HWND chosen_window;

void HandleDoubleClick() {
    if (IsWindowVisible(chosen_window)) {
        ShowWindow(chosen_window, SW_HIDE);
    } else {
        ShowWindow(chosen_window, SW_SHOW);
    }
}

struct handle_data {
    DWORD process_id;
    HWND window_handle;
};

BOOL is_main_window(HWND handle) {
    return GetWindow(handle, GW_OWNER) == (HWND) nullptr && IsWindowVisible(handle);
}

BOOL CALLBACK enum_windows_callback(HWND handle, LPARAM lParam) {
    handle_data &data = *(handle_data *) lParam;
    DWORD process_id = 0;
    GetWindowThreadProcessId(handle, &process_id);
    if (data.process_id != process_id || !is_main_window(handle)) {
        return TRUE;
    }
    data.window_handle = handle;
    return FALSE;
}

HWND find_main_window(DWORD process_id) {
    handle_data data{process_id, nullptr};
    EnumWindows(enum_windows_callback, (LPARAM) &data);
    return data.window_handle;
}

std::wstring *main_window_name;

HWND PrintProcessNameAndID(DWORD processID, int window_count) {
    HWND window_handle = nullptr;

    // Get a handle to the process.

    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION |
                                  PROCESS_VM_READ,
                                  FALSE, processID);

    // Get the process name.

    if (nullptr != hProcess) {
        HMODULE hMod;
        DWORD cbNeeded;

        if (EnumProcessModules(hProcess, &hMod, sizeof(HMODULE),
                               &cbNeeded)) {
//            GetModuleBaseName(hProcess, hMod, szProcessName,
//                              sizeof(szProcessName) / sizeof(TCHAR));
            HWND handle = find_main_window(processID);
            if (handle == nullptr) {
                goto skip;
            }
            main_window_name[window_count].resize(GetWindowTextLength(handle) + 1, L'\0');
            int nChar = GetWindowText(handle, reinterpret_cast<LPSTR>(&main_window_name[window_count][0]),
                                      static_cast<int>(main_window_name[window_count].size()));
            if (nChar < 1) {
                goto skip;
            }
            window_handle = handle;

            GetWindowThreadProcessId(handle, &processID);
        } else {
            goto skip;
        }
    } else {
        goto skip;
    }

    // Print the process name and identifier.
    _tprintf(TEXT("%i) %s  (PID: %u)\n"), window_count, main_window_name[window_count].c_str(), processID);

    skip:

    // Release the handle to the process.

    CloseHandle(hProcess);
    return window_handle;
}

#define TRAY_ID 1001
#define MY_TRAY_ICON_MESSAGE (WM_APP + 5)
#define IDM_EXIT 1005
#define IDM_EXIT_TRAY 1006

void ShowContextMenu(HWND hWnd) {

    POINT pt;
    GetCursorPos(&pt);

    HMENU hMenu = CreatePopupMenu();
    InsertMenu(hMenu, 0, MF_BYPOSITION | MF_STRING, IDM_EXIT, "Exit");
    InsertMenu(hMenu, 0, MF_BYPOSITION | MF_STRING, IDM_EXIT_TRAY, "Close tray icon");

    SetForegroundWindow(hWnd);

    int cmd = TrackPopupMenu(hMenu, TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_BOTTOMALIGN
                                    | TPM_NONOTIFY | TPM_RETURNCMD, pt.x, pt.y, 0, hWnd, nullptr);

    PostMessage(hWnd, WM_NULL, 0, 0);

    switch (cmd) {
        case IDM_EXIT:
            DestroyWindow(chosen_window);
        case IDM_EXIT_TRAY:
            DestroyWindow(hWnd);
            exit(0);
        default:
            // noop
            break;
    }
}

NOTIFYICONDATA niData = {};

LRESULT CALLBACK MyDlgProc(HWND hWnd, UINT message,
                           WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case MY_TRAY_ICON_MESSAGE:
            switch (LOWORD(lParam)) {
                case WM_LBUTTONDBLCLK:
                    HandleDoubleClick();
                    break;
                case WM_RBUTTONDOWN:
                case WM_CONTEXTMENU:
                    ShowContextMenu(hWnd);
                    break;
            }
            break;
        case WM_DESTROY:
            Shell_NotifyIcon(NIM_DELETE, &niData);
            break;
        default:
            // noop
            break;
    }
    return DefWindowProc(hWnd, message, wParam, lParam);
}

HWND hidden_dialog;

void create_system_tray(HWND window, const std::wstring &name) {
    // zero the structure - note: Some Windows funtions
    // require this but I can't be bothered to remember
    // which ones do and which ones don't.


    ZeroMemory(&niData, sizeof(NOTIFYICONDATA));

    niData.cbSize = sizeof(NOTIFYICONDATA);


    // the ID number can be any UINT you choose and will
    // be used to identify your icon in later calls to
    // Shell_NotifyIcon


    niData.uID = TRAY_ID;


    // state which structure members are valid
    // here you can also choose the style of tooltip
    // window if any - specifying a balloon window:
    // NIF_INFO is a little more complicated


    niData.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP | NIF_INFO | NIF_SHOWTIP;


    // load the icon note: you should destroy the icon
    // after the call to Shell_NotifyIcon
#define GCL_HICON (-14)

    niData.hIcon = (HICON) GetClassLongPtr(window, GCL_HICON);


    // set the window you want to recieve event messages

    HINSTANCE hInstance = GetModuleHandle(nullptr);

    static const char *class_name = "DUMMY_CLASS";
    WNDCLASSEX wx = {};
    wx.cbSize = sizeof(WNDCLASSEX);
    wx.lpfnWndProc = MyDlgProc;
    wx.hInstance = hInstance;
    wx.lpszClassName = class_name;
    if (RegisterClassEx(&wx)) {
        hidden_dialog = CreateWindowEx(0, class_name, "Helper window", 0, 0, 0, 0, 0, HWND_MESSAGE, nullptr, nullptr,
                                       nullptr);
    }

    niData.hWnd = hidden_dialog;


    // set the message to send
    // note: the message value should be in the
    // range of WM_APP through 0xBFFF


    niData.uCallbackMessage = MY_TRAY_ICON_MESSAGE;

    lstrcpy(niData.szTip, reinterpret_cast<LPCSTR>(name.c_str()));

    Shell_NotifyIcon(NIM_ADD, &niData);
    Shell_NotifyIcon(NIM_SETVERSION, &niData); //called only when usingNIM_ADD
}

#define WM_NAMECHANGED (WM_APP + 6)

void CALLBACK WinEventProc(
        HWINEVENTHOOK hWinEventHook,
        DWORD event,
        HWND hwnd,
        LONG idObject,
        LONG idChild,
        DWORD dwEventThread,
        DWORD dwmsEventTime
) {
    // Check this is the window we want. Titlebar name changes result in these
    // two values (obtained by looking at some titlebar changes with the
    // Accessible Event Watcher tool in the Windows SDK)
    if (hwnd == chosen_window) {
        // Do minimal work here, just hand off event to mainline.
        // If you do anything here that has a message loop - eg display a dialog or
        // messagebox, you can get reentrancy.
        PostThreadMessage(GetCurrentThreadId(), WM_NAMECHANGED, 0, 0);
    }
}

void clear_screen(char fill = ' ') {
    COORD tl = {0,0};
    CONSOLE_SCREEN_BUFFER_INFO s;
    HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
    GetConsoleScreenBufferInfo(console, &s);
    DWORD written, cells = s.dwSize.X * s.dwSize.Y;
    FillConsoleOutputCharacter(console, fill, cells, tl, &written);
    FillConsoleOutputAttribute(console, s.wAttributes, cells, tl, &written);
    SetConsoleCursorPosition(console, tl);
}

int main() {
    printf("These are all the windows currently open:\n");
    main_window_name = new std::wstring[100];
    {
        // Get the list of process identifiers.
        DWORD aProcesses[1024], cbNeeded, cProcesses;
        unsigned int i;

        if (!EnumProcesses(aProcesses, sizeof(aProcesses), &cbNeeded)) {
            return 1;
        }
        // Calculate how many process identifiers were returned.

        cProcesses = cbNeeded / sizeof(DWORD);

        // Print the name and process identifier for each process.
        HWND windows[1024];
        int window_count = 0;
        for (i = 0; i < cProcesses; i++) {
            if (aProcesses[i] != 0) {
                HWND handle = PrintProcessNameAndID(aProcesses[i], window_count);
                if (handle != nullptr) {
                    windows[window_count++] = handle;
                }
            }
        }

//        printf("Choose one (0 - %i): ", window_count - 1);
        std::cout << "Choose one (0 - " << window_count - 1 << ')' << std::endl;

        int chosen;
        std::cin >> chosen;

        create_system_tray(windows[chosen], main_window_name[chosen]);

        clear_screen();
        printf("You selected:\n%s\nWhat would you like to do?\n", main_window_name[chosen].c_str());
        printf("%i) Create tray icon\n", 1);

        delete[] main_window_name;
        chosen_window = windows[chosen];

        std::cin >> chosen;
        if(chosen != 1) return 0;
    }
    HWINEVENTHOOK hook = SetWinEventHook(EVENT_OBJECT_NAMECHANGE, EVENT_OBJECT_NAMECHANGE, nullptr, WinEventProc, 0,
                                         GetWindowThreadProcessId(chosen_window, nullptr), WINEVENT_OUTOFCONTEXT);

    ShowWindow(GetConsoleWindow(), SW_HIDE);

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        if (msg.message == WM_NAMECHANGED) {
            WCHAR szName[128];
            GetWindowText(chosen_window, (LPSTR) szName, ARRAYSIZE(szName));
            lstrcpy(niData.szTip, (LPSTR) szName);
            Shell_NotifyIcon(NIM_MODIFY, &niData);
        }
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    UnhookWinEvent(hook);
    Shell_NotifyIcon(NIM_DELETE, &niData);

    return 0;
}