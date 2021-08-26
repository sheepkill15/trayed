//
// Created by Aron on 8/25/2021.
//

#include <windows.h>
#include <cstdio>
#include <fstream>

#include "helper_Export.h"

#pragma data_seg("HOOKSEC")
HWND  hMaster = nullptr;
HHOOK hHook   = nullptr;
#pragma data_seg()
#pragma comment(linker, "/section:HOOKSEC,rws")

HINSTANCE hInst = nullptr;
UINT wmNotify;

LRESULT CALLBACK Inject(int nCode, WPARAM wp, LPARAM lp)
{
    std::ofstream log(R"(C:\Users\Aron\Documents\log.txt)");
    log << nCode << std::endl;
//    if(nCode == HCBT_DESTROYWND) {
//        PostMessage(hMaster, HCBT_DESTROYWND, 0, 0);
//    }
    return CallNextHookEx(hHook, nCode, wp, lp);
}

helper_EXPORT
BOOL SetHook(HWND hwndMaster, DWORD dwThreadId)
{
    hMaster = hwndMaster;
    hHook = SetWindowsHookEx( WH_CBT, Inject, hInst, dwThreadId );
    DWORD errorID = GetLastError();
    if (errorID != 0)
    {
        MessageBoxA(hwndMaster, "Failed to implement hook", "Failed", 0);
        MessageBoxA(hwndMaster, std::to_string(errorID).c_str(), "Error ID", 0);
        return false;
    }
    else
    {
        printf("%x\n", hHook);
        return (hHook != nullptr);
    }
}

helper_EXPORT
void Unhook()
{
    UnhookWindowsHookEx(hHook);
}

[[maybe_unused]] int DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID pReserved)
{
    switch(dwReason)
    {
        case DLL_PROCESS_ATTACH:
            hInst = hInstance;
            wmNotify = RegisterWindowMessage("MyHookNotifyMessage");
            break;
        case DLL_PROCESS_DETACH:
            PostMessage( hMaster, wmNotify, 0, 0 );
            break;
    }

    return 1;
}