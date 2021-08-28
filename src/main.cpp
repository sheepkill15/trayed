#include <windows.h>
#include <tchar.h>
#include <psapi.h>
#include <string>
#include <iostream>

#include "win_tray.h"
//#include "helper.h"

bool stop_process = false;

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
    handle_data chosen_window{};
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
        handle_data windows[1024];
        int window_count = 0;
        for (i = 0; i < cProcesses; i++) {
            if (aProcesses[i] != 0) {
                HWND handle = PrintProcessNameAndID(aProcesses[i], window_count);
                if (handle != nullptr) {
                    windows[window_count++] = {aProcesses[i], handle};
                }
            }
        }

//        printf("Choose one (0 - %i): ", window_count - 1);
        std::cout << "Choose one (0 - " << window_count - 1 << ')' << std::endl;

        int chosen;
        std::cin >> chosen;


//        clear_screen();
        printf("You selected:\n%s\nWhat would you like to do?\n", main_window_name[chosen].c_str());
        printf("%i) Create tray icon\n", 1);

        int option;
        std::cin >> option;
        if(option != 1) return 0;
        chosen_window = windows[chosen];

        delete[] main_window_name;
    }
    win_tray mytray(chosen_window);
//    if(SetHook(GetConsoleWindow(), GetWindowThreadProcessId(chosen_window, nullptr))) {
//        std::cout << "We did it we sons of a bitch" << std::endl;
//    }

    ShowWindow(GetConsoleWindow(), SW_HIDE);

    MSG msg;
    while (!mytray.is_closed()) {
        PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE);
//        if (msg.message == WM_NAMECHANGED) {
//            WCHAR szName[128];
//            GetWindowText(chosen_window.window_handle, (LPSTR) szName, ARRAYSIZE(szName));
//            lstrcpy(niData.szTip, (LPSTR) szName);
//            Shell_NotifyIcon(NIM_MODIFY, &niData);
//        }
//        else if(msg.message == HCBT_DESTROYWND ) {
//            DestroyWindow(hidden_dialog);
//            break;
//        }
        TranslateMessage(&msg);
        DispatchMessage(&msg);
        Sleep(250);
    }

//    Unhook();

    return 0;
}