#include "win.h"
#include <stdlib.h>
#include <string.h>
#ifndef BELOW_NORMAL_PRIORITY_CLASS
#define BELOW_NORMAL_PRIORITY_CLASS 0x4000
#define ABOVE_NORMAL_PRIORITY_CLASS 0x8000
#endif
typedef struct {
    WNDPROC previous;
    HMENU root, priority, alpha;
    UINT base;
    int installed, taking;
} MenuState;
static HINSTANCE module;
static HHOOK call_hook, mouse_hook;
static HANDLE mapping;
static WgShared *shared;
static HWND moving;
static int connect(void)
{
    if (shared) return !shared->stopping;
    mapping = OpenFileMappingA(FILE_MAP_ALL_ACCESS,FALSE,WG_MAP);
    if (!mapping) return 0;
    shared = (WgShared *)MapViewOfFile(mapping,FILE_MAP_ALL_ACCESS,0,0,sizeof(WgShared));
    return shared && !shared->stopping;
}
static LRESULT tell(HWND w, UINT event)
{
    DWORD_PTR result;
    if (!connect()) return 0;
    result = 0;
    SendMessageTimeoutA((HWND)(ULONG_PTR)shared->controller32,WG_MSG,
        (WPARAM)w,event,SMTO_ABORTIFHUNG | SMTO_BLOCK,1000,&result);
    return (LRESULT)result;
}
static int used(HMENU menu, UINT id)
{
    int i, n; HMENU sub;
    n = GetMenuItemCount(menu);
    for (i = 0; i < n; ++i) {
        if (GetMenuItemID(menu,i) == id) return 1;
        sub = GetSubMenu(menu,i);
        if (sub && used(sub,id)) return 1;
    }
    return 0;
}
static void remove_entries(MenuState *s)
{
    int i; UINT id; HMENU sub; char label[80];
    if (!s->installed || !IsMenu(s->root)) { s->installed = 0; return; }
    for (i = GetMenuItemCount(s->root)-1; i >= 0; --i) {
        sub = GetSubMenu(s->root,i); id = GetMenuItemID(s->root,i);
        label[0] = 0; GetMenuStringA(s->root,(UINT)i,label,sizeof(label),MF_BYPOSITION);
        if ((sub && (sub == s->priority || sub == s->alpha)) ||
            (id == s->base && !strcmp(label,"Always On Top")) ||
            (id == s->base+16 && !strcmp(label,"Minimize To Tray")))
            DeleteMenu(s->root,(UINT)i,MF_BYPOSITION);
    }
    s->priority = s->alpha = NULL; s->installed = 0;
}
static int install_entries(HWND w, MenuState *s)
{
    HMENU root; UINT base, id; int i, free_block; char text[32];
    static const char *names[] = {"Idle","Normal","High","Below Normal","Above Normal"};
    root = GetSystemMenu(w,FALSE);
    if (!root) return 0;
    if (s->installed && root == s->root && used(root,s->base)) return 1;
    remove_entries(s); s->root = root;
    for (base = 0x2000; base + WG_COMMANDS*16 < 0xf000; base += WG_COMMANDS*16) {
        free_block = 1;
        for (id = base; id < base + WG_COMMANDS*16; id += 16)
            if (used(root,id)) free_block = 0;
        if (free_block) break;
    }
    if (base + WG_COMMANDS*16 >= 0xf000) return 0;
    s->base = base; s->priority = CreatePopupMenu(); s->alpha = CreatePopupMenu();
    if (!s->priority || !s->alpha) {
        if (s->priority) DestroyMenu(s->priority);
        if (s->alpha) DestroyMenu(s->alpha);
        return 0;
    }
    for (i = 0; i < 5; ++i) AppendMenuA(s->priority,MF_STRING,base+(WG_IDLE+i)*16,names[i]);
    for (i = 0; i < 10; ++i) {
        wsprintfA(text,"%d%% transparent",i*10);
        AppendMenuA(s->alpha,MF_STRING,base+(WG_ALPHA+i)*16,text);
    }
    AppendMenuA(root,MF_STRING,base,"Always On Top");
    AppendMenuA(root,MF_STRING,base+16,"Minimize To Tray");
    AppendMenuA(root,MF_POPUP,(UINT_PTR)s->priority,"Priority (process)");
    AppendMenuA(root,MF_POPUP,(UINT_PTR)s->alpha,"Transparency");
    s->installed = 1; return 1;
}
static void refresh(HWND w, MenuState *s)
{
    int i; DWORD pid, priority; HANDLE process; DWORD classes[5]; LRESULT caps;
    if (!connect() || !install_entries(w,s)) return;
    caps = tell(w,WG_EVENT_QUERY);
    EnableMenuItem(s->root,s->base,MF_BYCOMMAND | (shared->top ? MF_ENABLED : MF_GRAYED));
    CheckMenuItem(s->root,s->base,MF_BYCOMMAND |
        ((GetWindowLong(w,GWL_EXSTYLE)&WS_EX_TOPMOST) ? MF_CHECKED : MF_UNCHECKED));
    EnableMenuItem(s->root,s->base+16,MF_BYCOMMAND |
        (shared->tray && shared->tray_available && caps ? MF_ENABLED : MF_GRAYED));
    classes[0]=IDLE_PRIORITY_CLASS; classes[1]=NORMAL_PRIORITY_CLASS; classes[2]=HIGH_PRIORITY_CLASS;
    classes[3]=BELOW_NORMAL_PRIORITY_CLASS; classes[4]=ABOVE_NORMAL_PRIORITY_CLASS;
    GetWindowThreadProcessId(w,&pid); priority = 0;
    process = OpenProcess(PROCESS_QUERY_INFORMATION,FALSE,pid);
    if (process) { priority = GetPriorityClass(process); CloseHandle(process); }
    for (i=0;i<5;++i) {
        EnableMenuItem(s->priority,s->base+(WG_IDLE+i)*16,MF_BYCOMMAND |
            (shared->priority && (i<3 || shared->extended_priority) ? MF_ENABLED : MF_GRAYED));
        CheckMenuItem(s->priority,s->base+(WG_IDLE+i)*16,MF_BYCOMMAND |
            (priority == classes[i] ? MF_CHECKED : MF_UNCHECKED));
    }
    for (i=0;i<10;++i) EnableMenuItem(s->alpha,s->base+(WG_ALPHA+i)*16,
        MF_BYCOMMAND | (shared->transparency && shared->layered && (caps&2) ? MF_ENABLED : MF_GRAYED));
}
static LRESULT CALLBACK target_proc(HWND w, UINT msg, WPARAM wp, LPARAM lp)
{
    MenuState *s; WNDPROC previous; UINT id; LRESULT result;
    s = (MenuState *)GetPropA(w,WG_MENU);
    if (!s) return DefWindowProcA(w,msg,wp,lp);
    previous = s->previous;
    if (msg == WM_NCDESTROY) {
        tell(w,WG_EVENT_GONE); remove_entries(s);
        RemovePropA(w,WG_PROP); RemovePropA(w,WG_MENU);
        SetWindowLongPtrA(w,GWLP_WNDPROC,(LONG_PTR)previous);
        free(s); return CallWindowProcA(previous,w,msg,wp,lp);
    }
    if (!connect()) {
        remove_entries(s);
        if ((WNDPROC)GetWindowLongPtrA(w,GWLP_WNDPROC) == target_proc) {
            SetWindowLongPtrA(w,GWLP_WNDPROC,(LONG_PTR)previous);
            RemovePropA(w,WG_MENU); RemovePropA(w,WG_PROP); free(s);
        }
        return CallWindowProcA(previous,w,msg,wp,lp);
    }
    if (msg == WG_ATTACH && wp == WG_EVENT_REMOVE) {
        remove_entries(s); return 0;
    }
    if (msg == WM_SYSCOMMAND && s->installed) {
        id = (UINT)wp & 0xfff0;
        if (id >= s->base && id < s->base+WG_COMMANDS*16) {
            tell(w,(id-s->base)/16); return 0;
        }
    }
    if (msg == WG_ATTACH && wp == WG_EVENT_TAKEOVER) {
        s->taking = 1;
        SendMessageA(w,WM_CANCELMODE,0,0);
        ReleaseCapture(); s->taking = 0; return 0;
    }
    if (msg == WM_ENTERSIZEMOVE) { moving = w; tell(w,WG_EVENT_MOVE); }
    if (msg == WM_CAPTURECHANGED && !s->taking) tell(w,WG_EVENT_CAPTURE);
    if (msg == WM_EXITSIZEMOVE) { if (moving == w) moving = NULL; }
    /* Let applications rebuild their system menu before repairing our entries. */
    result = CallWindowProcA(previous,w,msg,wp,lp);
    if (msg == WM_INITMENU || msg == WM_INITMENUPOPUP || msg == WG_ATTACH) refresh(w,s);
    return result;
}
static void attach(HWND w)
{
    MenuState *s; char path[MAX_PATH];
    if (!connect() || !wg_eligible(w,shared->pid) || GetPropA(w,WG_MENU)) return;
    s = (MenuState *)calloc(1,sizeof(*s)); if (!s) return;
    s->previous = (WNDPROC)GetWindowLongPtrA(w,GWLP_WNDPROC);
    if (!SetPropA(w,WG_MENU,(HANDLE)s) ||
        !SetPropA(w,WG_PROP,(HANDLE)(ULONG_PTR)(shared->cookie ^ GetTickCount() ^ (DWORD)(ULONG_PTR)w))) {
        RemovePropA(w,WG_MENU); free(s); return;
    }
    SetLastError(0);
    if (!SetWindowLongPtrA(w,GWLP_WNDPROC,(LONG_PTR)target_proc) && GetLastError()) {
        RemovePropA(w,WG_MENU); RemovePropA(w,WG_PROP); free(s); return;
    }
    /* Keep callback code mapped even when a later subclass owns the chain. */
    GetModuleFileNameA(module,path,sizeof(path)); LoadLibraryA(path);
    refresh(w,s);
}
static LRESULT CALLBACK call_proc(int code, WPARAM wp, LPARAM lp)
{
    CWPSTRUCT *c;
    if (code >= 0) { c = (CWPSTRUCT *)lp; attach(c->hwnd); }
    return CallNextHookEx(call_hook,code,wp,lp);
}
static LRESULT CALLBACK mouse_proc(int code, WPARAM wp, LPARAM lp)
{
    MOUSEHOOKSTRUCT *m;
    if (code >= 0 && (wp == WM_RBUTTONDOWN || wp == WM_NCRBUTTONDOWN) &&
        (GetAsyncKeyState(VK_LBUTTON)&0x8000)) {
        m = (MOUSEHOOKSTRUCT *)lp;
        (void)m;
        if (moving && tell(moving,WG_EVENT_RIGHT)) return 1;
    }
    return CallNextHookEx(mouse_hook,code,wp,lp);
}
__declspec(dllexport) BOOL WINAPI WgInstall(void)
{
    if (!connect()) return FALSE;
    call_hook = SetWindowsHookExA(WH_CALLWNDPROC,call_proc,module,0);
    mouse_hook = SetWindowsHookExA(WH_MOUSE,mouse_proc,module,0);
    if (!call_hook || !mouse_hook) {
        if (call_hook) UnhookWindowsHookEx(call_hook);
        if (mouse_hook) UnhookWindowsHookEx(mouse_hook);
        call_hook = mouse_hook = NULL; return FALSE;
    }
    return TRUE;
}
__declspec(dllexport) void WINAPI WgUninstall(void)
{
    if (call_hook) UnhookWindowsHookEx(call_hook);
    if (mouse_hook) UnhookWindowsHookEx(mouse_hook);
    call_hook = mouse_hook = NULL;
}
BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved)
{
    (void)reserved;
    if (reason == DLL_PROCESS_ATTACH) module = instance;
    if (reason == DLL_PROCESS_DETACH) {
        if (shared) UnmapViewOfFile(shared);
        if (mapping) CloseHandle(mapping);
    }
    return TRUE;
}
