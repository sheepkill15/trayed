//
// Created by Aron on 8/28/2021.
//

#ifndef TRAYED_WIN_TRAY_H
#define TRAYED_WIN_TRAY_H

#include <windows.h>
#define TRAY_ID (WM_APP + 1)
#define MY_TRAY_ICON_MESSAGE (WM_APP + 2)
#define IDM_EXIT (WM_APP + 3)
#define IDM_EXIT_TRAY (WM_APP + 4)
#define WM_NAMECHANGED (WM_APP + 5)



struct handle_data {
    DWORD process_id;
    HWND window_handle;
};

class win_tray {
public:
    explicit win_tray(const handle_data& new_window);
    ~win_tray();

    static LRESULT CALLBACK wnd_proc(HWND window, UINT message, WPARAM wParam, LPARAM lParam);
    static void CALLBACK wnd_changed_proc(HWINEVENTHOOK hWinEventHook,
                                   DWORD event,
                                   HWND hwnd,
                                   LONG idObject,
                                   LONG idChild,
                                   DWORD dwEventThread,
                                   DWORD dwmsEventTime);

    [[nodiscard]] inline bool is_closed() const { return closed; };

private:
    void handle_tray_dbclick() const;
    void show_context_menu(HWND context);
    void create_hidden_window();
    void create_system_tray();
    void update_title();
    void cleanup();

private:
    HWND hidden_window = {};
    NOTIFYICONDATA ni_data = {};
    handle_data tracked_window = {};
    HWINEVENTHOOK namechanged_hook = {};

    bool closed = false;
};


#endif //TRAYED_WIN_TRAY_H
