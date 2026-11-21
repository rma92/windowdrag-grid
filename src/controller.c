#include "win.h"
#include <string.h>
#include <stdlib.h>
static HWND control, target;
static int testing;
static WgShared *state;
static WgSettings settings;
static WgGesture gesture;
static DWORD target_pid, target_cookie;
static HMODULE hooks;
static void (WINAPI *uninstall)(void);
static char directory[MAX_PATH], ini[MAX_PATH];
static void workarea(POINT p, WgRect *result)
{
    typedef HANDLE (WINAPI *MonitorPoint)(POINT,DWORD);
    typedef BOOL (WINAPI *MonitorInfo)(HANDLE,void *);
    struct { DWORD size; RECT monitor,work; DWORD flags; } info;
    MonitorPoint mp; MonitorInfo mi; RECT r;
    HMODULE user; user=GetModuleHandleA("user32.dll");
    mp=(MonitorPoint)GetProcAddress(user,"MonitorFromPoint");
    mi=(MonitorInfo)GetProcAddress(user,"GetMonitorInfoA");
    info.size=sizeof(info);
    if (mp && mi && mi(mp(p,2),&info)) r=info.work;
    else if (!SystemParametersInfoA(SPI_GETWORKAREA,0,&r,0)) {
        r.left=r.top=0; r.right=GetSystemMetrics(SM_CXSCREEN); r.bottom=GetSystemMetrics(SM_CYSCREEN);
    }
    result->left=r.left; result->top=r.top; result->right=r.right; result->bottom=r.bottom;
}
static void place(WgRect r)
{
    if (wg_identity(target,target_pid,target_cookie))
        SetWindowPos(target,NULL,(int)r.left,(int)r.top,(int)(r.right-r.left),
            (int)(r.bottom-r.top),SWP_NOACTIVATE|SWP_NOZORDER);
}
static void finish(int cancel)
{ wg_overlay_hide(); if (gesture.active) place(wg_finish(&gesture,cancel)); }
static void activate(HWND w)
{
    RECT r; POINT p; WgRect original, area; WgGrid grid; DWORD_PTR ignored;
    if (!settings.grid || gesture.active || (!(testing && w==control) && !wg_eligible(w,GetCurrentProcessId())) ||
        !(GetWindowLong(w,GWL_STYLE)&WS_THICKFRAME)) return;
    target=w; GetWindowThreadProcessId(w,&target_pid);
    target_cookie=(DWORD)(ULONG_PTR)GetPropA(w,WG_PROP);
    if (!target_cookie || !GetWindowRect(w,&r) || !GetCursorPos(&p)) return;
    original.left=r.left; original.top=r.top; original.right=r.right; original.bottom=r.bottom;
    grid.columns=settings.columns; grid.rows=settings.rows;
    workarea(p,&area);
    wg_begin(&gesture,grid,original,area,p.x,p.y);
    SendMessageTimeoutA(w,WG_ATTACH,WG_EVENT_TAKEOVER,0,SMTO_ABORTIFHUNG,1000,&ignored);
    if (!testing) wg_overlay_show(&gesture,&settings);
}
static BOOL CALLBACK attach_window(HWND w, LPARAM unused)
{
    (void)unused;
    if (wg_eligible(w,GetCurrentProcessId())) PostMessageA(w,WG_ATTACH,WG_EVENT_INSTALL,0);
    return TRUE;
}
static void command(HWND w, int id)
{
    HANDLE process; DWORD pid, classes[5];
    if (!wg_eligible(w,GetCurrentProcessId())) return;
    if (id>=WG_ALPHA && id<WG_COMMANDS && settings.transparency) {
        if (!wg_alpha_apply(w,id-WG_ALPHA)) MessageBoxA(control,"Unable to apply transparency safely.","WindowDrag Grid",MB_OK|MB_ICONERROR);
        return;
    }
    if (id==WG_TOP && settings.top) {
        SetWindowPos(w,(GetWindowLong(w,GWL_EXSTYLE)&WS_EX_TOPMOST)?HWND_NOTOPMOST:HWND_TOPMOST,
            0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE); return;
    }
    if (id>=WG_IDLE && id<=WG_ABOVE && settings.priority) {
        classes[0]=IDLE_PRIORITY_CLASS; classes[1]=NORMAL_PRIORITY_CLASS;
        classes[2]=HIGH_PRIORITY_CLASS; classes[3]=0x4000; classes[4]=0x8000;
        GetWindowThreadProcessId(w,&pid); process=OpenProcess(PROCESS_SET_INFORMATION,FALSE,pid);
        if (!process || !SetPriorityClass(process,classes[id-WG_IDLE]))
            MessageBoxA(control,"Unable to change process priority. Access may be denied.","WindowDrag Grid",MB_OK|MB_ICONERROR);
        if (process) CloseHandle(process);
    }
}
static void gesture_tick(void)
{
    POINT p; WgRect area; int right;
        if (gesture.active) {
            if (!wg_identity(target,target_pid,target_cookie)) { gesture.active=0; wg_overlay_hide(); return; }
            if (!testing && (GetAsyncKeyState(VK_ESCAPE)&0x8000)) finish(1);
            else if (!testing && !(GetAsyncKeyState(VK_LBUTTON)&0x8000)) finish(0);
            else if (GetCursorPos(&p)) {
                workarea(p,&area);
                right=(GetAsyncKeyState(VK_RBUTTON)&0x8000)!=0;
                wg_update(&gesture,area,p.x,p.y,right);
                if (settings.live) place(gesture.selection);
                if (!testing) wg_overlay_show(&gesture,&settings);
            }
        }
}
static LRESULT CALLBACK controller_proc(HWND w,UINT msg,WPARAM wp,LPARAM lp)
{
    POINT p; HMENU menu; int selected;
    if (msg==WG_MSG) {
        if (lp==WG_EVENT_QUERY) return wg_alpha_supported((HWND)wp)?2:0;
        if (lp==WG_EVENT_RIGHT && settings.grid) { PostMessageA(w,WG_ATTACH,wp,0); return 1; }
        if (lp<WG_COMMANDS) { PostMessageA(w,WM_APP+80,wp,lp); return 1; }
        return 0;
    }
    if (msg==WG_ATTACH) { activate((HWND)wp); return 0; }
    if (msg==WM_APP+80) { command((HWND)wp,(int)lp); return 0; }
    if (msg==WM_TIMER) {
        if (wp==2) { EnumWindows(attach_window,0); return 0; }
        gesture_tick();
        return 0;
    }
    if (msg==WM_DISPLAYCHANGE) finish(1);
    if (msg==WM_CONTEXTMENU) {
        GetCursorPos(&p); menu=CreatePopupMenu();
        AppendMenuA(menu,MF_STRING,1,settings.grid?"Pause Grid":"Resume Grid");
        AppendMenuA(menu,MF_STRING,2,"Save settings"); AppendMenuA(menu,MF_STRING,3,"Exit");
        selected=TrackPopupMenu(menu,TPM_RETURNCMD|TPM_RIGHTBUTTON,p.x,p.y,0,w,NULL);
        DestroyMenu(menu);
        if (selected==1) { finish(1); settings.grid=!settings.grid; state->grid=settings.grid; }
        if (selected==2 && !wg_save_settings(&settings,ini)) MessageBoxA(w,"Cannot save INI settings.","WindowDrag Grid",MB_OK|MB_ICONERROR);
        if (selected==3) DestroyWindow(w);
        return 0;
    }
    if (msg==WM_DESTROY) {
        finish(1); wg_alpha_restore(); state->stopping=1; if (uninstall) uninstall(); PostQuitMessage(0); return 0;
    }
    return DefWindowProcA(w,msg,wp,lp);
}
int WINAPI WinMain(HINSTANCE instance,HINSTANCE previous,LPSTR args,int show)
{
    HANDLE mapping, singleton; HWND existing; WNDCLASSA wc; MSG msg; char path[MAX_PATH]; char *slash;
    BOOL (WINAPI *install)(void);
    (void)previous; (void)show; (void)args; testing=strstr(GetCommandLineA(),"--self-test")!=NULL;
    singleton=CreateMutexA(NULL,FALSE,"WindowDragGrid.ControllerMutex.v1");
    if (!singleton) { MessageBoxA(NULL,"Cannot create controller mutex.","WindowDrag Grid",MB_OK|MB_ICONERROR); return 1; }
    if (GetLastError()==ERROR_ALREADY_EXISTS) {
        existing=FindWindowA(WG_CLASS,NULL);
        if (existing) { ShowWindow(existing,SW_RESTORE); SetForegroundWindow(existing); }
        else MessageBoxA(NULL,"Another controller is starting or shutting down.","WindowDrag Grid",MB_OK);
        CloseHandle(singleton); return 0;
    }
    mapping=CreateFileMappingA(INVALID_HANDLE_VALUE,NULL,PAGE_READWRITE,0,sizeof(WgShared),WG_MAP);
    if (!mapping) return 1;
    state=(WgShared *)MapViewOfFile(mapping,FILE_MAP_ALL_ACCESS,0,0,sizeof(WgShared));
    if (!state) { CloseHandle(mapping); return 1; }
    memset(state,0,sizeof(*state)); state->pid=GetCurrentProcessId(); state->cookie=GetTickCount();
    GetModuleFileNameA(instance,directory,sizeof(directory)); slash=strrchr(directory,'\\');
    if (!slash) return 1; slash[1]=0;
    lstrcpyA(ini,directory); lstrcatA(ini,"windowdrag-grid.ini"); wg_load_settings(&settings,ini);
    state->grid=settings.grid; state->top=settings.top; state->priority=settings.priority;
    state->transparency=settings.transparency; state->layered=wg_alpha_init();
    memset(&wc,0,sizeof(wc)); wc.lpfnWndProc=controller_proc; wc.hInstance=instance;
    wc.lpszClassName=WG_CLASS; wc.hCursor=LoadCursor(NULL,IDC_ARROW);
    wc.hbrBackground=(HBRUSH)(COLOR_BTNFACE+1); RegisterClassA(&wc);
    control=CreateWindowA(WG_CLASS,"WindowDrag Grid - right-click for controls",WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,CW_USEDEFAULT,480,160,NULL,NULL,instance,NULL);
    state->controller32=(DWORD)(ULONG_PTR)control;
    lstrcpyA(path,directory);
#ifdef _WIN64
    lstrcatA(path,"wg-hook64.dll");
#else
    lstrcatA(path,"wg-hook32.dll");
#endif
    hooks=LoadLibraryA(path);
    install=hooks?(BOOL (WINAPI *)(void))GetProcAddress(hooks,"WgInstall"):NULL;
    uninstall=hooks?(void (WINAPI *)(void))GetProcAddress(hooks,"WgUninstall"):NULL;
    if (hooks && !install) install=(BOOL (WINAPI *)(void))GetProcAddress(hooks,"_WgInstall@0");
    if (hooks && !uninstall) uninstall=(void (WINAPI *)(void))GetProcAddress(hooks,"_WgUninstall@0");
    if (!testing && (!install || !install())) {
        char error[640]; DWORD code;
        code=GetLastError();
        wsprintfA(error,"%s\r\n%s\r\nWindows error: %lu",
            !hooks?"Cannot load hook DLL":!install?"Hook DLL has no install export":"Hook installation failed",path,code);
        MessageBoxA(NULL,error,"WindowDrag Grid",MB_OK|MB_ICONERROR);
        state->stopping=1;
        if (hooks) FreeLibrary(hooks);
        UnmapViewOfFile(state); CloseHandle(mapping); CloseHandle(singleton); return 1;
    }
    if (testing) {
        SetPropA(control,WG_PROP,(HANDLE)123);
        SendMessageA(control,WG_ATTACH,(WPARAM)control,0);
        SendMessageA(control,WM_TIMER,1,0);
        wg_overlay_show(&gesture,&settings); wg_overlay_hide();
        finish(1); DestroyWindow(control);
        if (hooks) FreeLibrary(hooks);
        UnmapViewOfFile(state); CloseHandle(mapping); CloseHandle(singleton);
        return 0;
    }
    SetTimer(control,1,16,NULL); SetTimer(control,2,2000,NULL);
    EnumWindows(attach_window,0); ShowWindow(control,SW_SHOWNORMAL);
    while (GetMessageA(&msg,NULL,0,0)>0) { TranslateMessage(&msg); DispatchMessageA(&msg); }
    if (hooks) FreeLibrary(hooks); UnmapViewOfFile(state); CloseHandle(mapping); CloseHandle(singleton); return 0;
}









