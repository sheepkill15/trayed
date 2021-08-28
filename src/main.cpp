#include <windows.h>
#include <tchar.h>
#include <psapi.h>
#include <cstdio>

#include "win_tray.h"

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

HWND PrintProcessNameAndID(DWORD processID, int window_count) {
    HWND window_handle = nullptr;

    // Get a handle to the process.

    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION |
                                  PROCESS_VM_READ,
                                  FALSE, processID);

    WCHAR name[128];
    if (nullptr != hProcess) {
        HMODULE hMod;
        DWORD cbNeeded;

        if (EnumProcessModules(hProcess, &hMod, sizeof(HMODULE),
                               &cbNeeded)) {
            HWND handle = find_main_window(processID);
            if (handle == nullptr) {
                goto skip;
            }
            int nChar = GetWindowText(handle, reinterpret_cast<LPSTR>(&name[0]), 128);
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
    _tprintf(TEXT("%i) %s  (PID: %u)\n"), window_count, name, processID);

    skip:

    // Release the handle to the process.

    CloseHandle(hProcess);
    return window_handle;
}

int main() {
    printf("These are all the windows currently open:\n");
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

        printf("Choose one (0 - %i)\n", window_count - 1);
        int chosen;
        scanf("%d", &chosen); // NOLINT(cert-err34-c)
        chosen_window = windows[chosen];
        WCHAR name[128];
        GetWindowText(chosen_window.window_handle, reinterpret_cast<LPSTR>(&name[0]), 128);
        printf("You selected:\n%s\nWhat would you like to do?\n", name);
        printf("%i) Create tray icon\n", 1);

        int option;
        scanf("%d", &option);
        if(option != 1) return 0;
    }
    win_tray mytray(chosen_window);

    ShowWindow(GetConsoleWindow(), SW_HIDE);

    MSG msg;
    while (!mytray.is_closed()) {
        PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE);
        TranslateMessage(&msg);
        DispatchMessage(&msg);
        Sleep(250);
    }

    return 0;
}