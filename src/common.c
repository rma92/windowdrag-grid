#include "win.h"
#include <stdlib.h>
int wg_eligible(HWND w, DWORD own_pid)
{
    DWORD pid; LONG style;
    if (!IsWindow(w) || GetParent(w) || !IsWindowVisible(w) || IsIconic(w)) return 0;
    GetWindowThreadProcessId(w, &pid);
    style = GetWindowLong(w, GWL_STYLE);
    return pid != own_pid && w != GetDesktopWindow() &&
        (style & WS_SYSMENU) && !(style & WS_CHILD) &&
        !(GetWindowLong(w, GWL_EXSTYLE) & WS_EX_TOOLWINDOW);
}
int wg_identity(HWND w, DWORD pid, DWORD cookie)
{
    DWORD actual;
    if (!IsWindow(w)) return 0;
    GetWindowThreadProcessId(w, &actual);
    return actual == pid && (DWORD)(ULONG_PTR)GetPropA(w, WG_PROP) == cookie;
}
static int setting(const char *p, const char *key, int def)
{ return (int)GetPrivateProfileIntA("WindowDragGrid", key, def, p); }
void wg_load_settings(WgSettings *s, const char *p)
{
    s->grid = !!setting(p,"Grid",1); s->top = !!setting(p,"Topmost",1);
    s->tray = !!setting(p,"Tray",1); s->priority = !!setting(p,"Priority",1);
    s->transparency = !!setting(p,"Transparency",1);
    s->columns = setting(p,"Columns",12); s->rows = setting(p,"Rows",8);
    if (s->columns < 1 || s->columns > 64) s->columns = 12;
    if (s->rows < 1 || s->rows > 64) s->rows = 8;
    s->live = !!setting(p,"Live",1); s->thickness = setting(p,"Thickness",1);
    if (s->thickness < 1 || s->thickness > 8) s->thickness = 1;
    s->tray_mode = setting(p,"TrayMode",0);
    if (s->tray_mode < 0 || s->tray_mode > 2) s->tray_mode = 0;
    s->grid_color = (COLORREF)setting(p,"GridColor",RGB(80,120,180));
    s->selection_color = (COLORREF)setting(p,"SelectionColor",RGB(255,160,20));
}
static int put(const char *p, const char *key, long value)
{
    char buf[32]; wsprintfA(buf,"%ld",value);
    return WritePrivateProfileStringA("WindowDragGrid",key,buf,p) != 0;
}
int wg_save_settings(const WgSettings *s, const char *p)
{
    int ok; ok = 1;
#define SAVE(key,field) if (!put(p,key,(long)s->field)) ok = 0
    SAVE("Grid",grid); SAVE("Topmost",top); SAVE("Tray",tray);
    SAVE("Priority",priority); SAVE("Transparency",transparency);
    SAVE("Columns",columns); SAVE("Rows",rows); SAVE("Live",live);
    SAVE("Thickness",thickness); SAVE("TrayMode",tray_mode);
    SAVE("GridColor",grid_color); SAVE("SelectionColor",selection_color);
#undef SAVE
    return ok;
}
