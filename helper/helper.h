//
// Created by Aron on 8/25/2021.
//

#ifndef TRAYED_HELPER_H
#define TRAYED_HELPER_H

#include "windows.h"

__declspec(dllimport)
BOOL SetHook(HWND hwndMaster, DWORD dwThreadId);
__declspec(dllimport)
void Unhook();


#endif //TRAYED_HELPER_H
