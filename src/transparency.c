#include "win.h"
#ifndef WS_EX_LAYERED
#define WS_EX_LAYERED 0x80000
#endif
typedef struct {
    HWND window; DWORD pid,cookie; LONG style;
    COLORREF key; BYTE alpha,last; DWORD flags;
} AlphaEntry;
static AlphaEntry entries[WG_MAX];
static WgSetAlpha set_alpha;
static WgGetAlpha get_alpha;
int wg_alpha_init(void)
{
    HMODULE user; user=GetModuleHandleA("user32.dll");
    set_alpha=(WgSetAlpha)GetProcAddress(user,"SetLayeredWindowAttributes");
    get_alpha=(WgGetAlpha)GetProcAddress(user,"GetLayeredWindowAttributes");
    return set_alpha && get_alpha;
}
int wg_alpha_supported(HWND w)
{
    COLORREF key; BYTE alpha; DWORD flags;
    if (!set_alpha || !get_alpha) return 0;
    if (!(GetWindowLong(w,GWL_EXSTYLE)&WS_EX_LAYERED)) return 1;
    return get_alpha(w,&key,&alpha,&flags) && flags==2;
}
int wg_alpha_apply(HWND w,int level)
{
    int i,slot; AlphaEntry *e; BYTE alpha;
    if (level<0 || level>9 || !wg_alpha_supported(w)) return 0;
    slot=-1;
    for (i=0;i<WG_MAX;++i) {
        if (entries[i].window && !wg_identity(entries[i].window,entries[i].pid,entries[i].cookie)) entries[i].window=NULL;
        if (entries[i].window==w) { slot=i; break; }
        if (!entries[i].window) slot=i;
    }
    if (slot<0) return 0;
    e=&entries[slot];
    if (!e->window) {
        e->style=GetWindowLong(w,GWL_EXSTYLE); e->alpha=255; e->flags=2; e->key=0;
        if ((e->style&WS_EX_LAYERED) && !get_alpha(w,&e->key,&e->alpha,&e->flags)) return 0;
        GetWindowThreadProcessId(w,&e->pid); e->cookie=(DWORD)(ULONG_PTR)GetPropA(w,WG_PROP);
        if (!e->cookie) return 0;
        e->window=w;
    }
    alpha=(BYTE)((255*(100-level*10)+50)/100);
    SetLastError(0);
    if (!(GetWindowLong(w,GWL_EXSTYLE)&WS_EX_LAYERED)) {
        SetWindowLong(w,GWL_EXSTYLE,GetWindowLong(w,GWL_EXSTYLE)|WS_EX_LAYERED);
        if (GetLastError()) return 0;
    }
    if (!set_alpha(w,0,alpha,2)) {
        if (!(e->style&WS_EX_LAYERED)) SetWindowLong(w,GWL_EXSTYLE,GetWindowLong(w,GWL_EXSTYLE)&~WS_EX_LAYERED);
        e->window=NULL; return 0;
    }
    e->last=alpha; return 1;
}
void wg_alpha_restore(void)
{
    int i; AlphaEntry *e; COLORREF key; BYTE alpha; DWORD flags;
    for (i=0;i<WG_MAX;++i) {
        e=&entries[i];
        if (e->window && wg_identity(e->window,e->pid,e->cookie) &&
            get_alpha(e->window,&key,&alpha,&flags) && flags==2 && key==0 && alpha==e->last) {
            if (e->style&WS_EX_LAYERED) set_alpha(e->window,e->key,e->alpha,e->flags);
            else SetWindowLong(e->window,GWL_EXSTYLE,GetWindowLong(e->window,GWL_EXSTYLE)&~WS_EX_LAYERED);
        }
        e->window=NULL;
    }
}
