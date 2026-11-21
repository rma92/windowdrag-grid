#ifndef WG_WIN_H
#define WG_WIN_H
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "core.h"
#define WG_CLASS "WindowDragGrid.Controller.v1"
#define WG_MAP "WindowDragGrid.State.v1"
#define WG_PROP "WindowDragGrid.Identity.v1"
#define WG_OLDPROC "WindowDragGrid.OldProc.v1"
#define WG_MENU "WindowDragGrid.Menu.v1"
#define WG_MSG (WM_APP+73)
#define WG_TRAY (WM_APP+74)
#define WG_ATTACH (WM_APP+75)
#define WG_MAX 128
#define WG_TOP 0
#define WG_HIDE 1
#define WG_IDLE 2
#define WG_NORMAL 3
#define WG_HIGH 4
#define WG_BELOW 5
#define WG_ABOVE 6
#define WG_ALPHA 7
#define WG_COMMANDS 17
#define WG_EVENT_MOVE 100
#define WG_EVENT_RIGHT 101
#define WG_EVENT_CAPTURE 102
#define WG_EVENT_GONE 103
#define WG_EVENT_QUERY 104
#define WG_EVENT_TAKEOVER 105
#define WG_EVENT_FINISH 106
#define WG_EVENT_INSTALL 107
#define WG_EVENT_REMOVE 108
typedef struct {
    DWORD hwnd32, pid, cookie;
    volatile LONG hidden;
    WINDOWPLACEMENT placement;
} WgRecoveryEntry;
/* No pointers or pointer-width handles: same layout for both architectures. */
typedef struct {
    DWORD controller32, pid, cookie;
    volatile LONG stopping;
    int grid, top, tray, priority, transparency;
    int tray_available, layered, extended_priority;
    volatile LONG recovery_ready;
    WgRecoveryEntry entries[WG_MAX];
} WgShared;
typedef struct {
    int grid, top, tray, priority, transparency;
    int columns, rows, live, thickness, tray_mode;
    COLORREF grid_color, selection_color;
} WgSettings;
typedef BOOL (WINAPI *WgNotify)(DWORD, void *);
typedef BOOL (WINAPI *WgSetAlpha)(HWND,COLORREF,BYTE,DWORD);
typedef BOOL (WINAPI *WgGetAlpha)(HWND,COLORREF *,BYTE *,DWORD *);
int wg_eligible(HWND w, DWORD own_pid);
int wg_identity(HWND w, DWORD pid, DWORD cookie);
void wg_load_settings(WgSettings *s, const char *path);
int wg_save_settings(const WgSettings *s, const char *path);
int wg_recover(const char *pid);
int wg_alpha_init(void);
int wg_alpha_supported(HWND w);
int wg_alpha_apply(HWND w,int level);
void wg_alpha_restore(void);
void wg_overlay_show(const WgGesture *g,const WgSettings *s);
void wg_overlay_hide(void);
#endif
