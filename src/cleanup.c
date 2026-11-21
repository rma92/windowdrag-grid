/* C89, both bitnesses. Never calls FreeLibrary in a foreign process. */
#include "win.h"
#include <stdio.h>
static unsigned requested, timed_out, remaining;
static BOOL CALLBACK clean_window(HWND w, LPARAM unused)
{
    DWORD_PTR result; (void)unused;
    if (!GetPropA(w,WG_MENU)) return TRUE;
    ++requested;
    result=0;
    if (!SendMessageTimeoutA(w,WG_ATTACH,WG_EVENT_REMOVE,0,
        SMTO_ABORTIFHUNG|SMTO_BLOCK,1500,&result)) ++timed_out;
    /* A second message lets older callbacks observe stopping and detach. */
    SendMessageTimeoutA(w,WM_NULL,0,0,SMTO_ABORTIFHUNG|SMTO_BLOCK,1500,&result);
    if (GetPropA(w,WG_MENU)) ++remaining;
    return TRUE;
}
int main(void)
{
    HANDLE mapping, process; WgShared *shared; HWND controller;
    DWORD pid; DWORD_PTR result;
    mapping=OpenFileMappingA(FILE_MAP_ALL_ACCESS,FALSE,WG_MAP);
    shared=mapping?(WgShared *)MapViewOfFile(mapping,FILE_MAP_ALL_ACCESS,0,0,sizeof(WgShared)):NULL;
    controller=FindWindowA(WG_CLASS,NULL); process=NULL;
    if (controller) {
        GetWindowThreadProcessId(controller,&pid);
        process=OpenProcess(SYNCHRONIZE,FALSE,pid);
        result=0;
        if (!SendMessageTimeoutA(controller,WM_CLOSE,0,0,
            SMTO_ABORTIFHUNG|SMTO_BLOCK,3000,&result)) {
            puts("Controller did not respond. No forced termination performed.");
            if (process) CloseHandle(process);
            if (shared) UnmapViewOfFile(shared);
            if (mapping) CloseHandle(mapping);
            return 1;
        }
        if (process) {
            if (WaitForSingleObject(process,5000)!=WAIT_OBJECT_0) {
                puts("Controller has not exited. Try cleanup again after it closes.");
                CloseHandle(process);
                if (shared) UnmapViewOfFile(shared);
                if (mapping) CloseHandle(mapping);
                return 1;
            }
            CloseHandle(process);
        }
    }
    if (shared) InterlockedExchange(&shared->stopping,1);
    EnumWindows(clean_window,0);
    printf("Cleanup requested for %u windows; %u timed out; %u still subclassed.\n",
        requested,timed_out,remaining);
    puts("The prototype pins its hook DLL in each target process.");
    puts("Menus/subclasses can detach, but DLL files remain loaded until those applications exit.");
    puts("Close and reopen affected applications to release the DLL files. No force-unload attempted.");
    if (shared) UnmapViewOfFile(shared);
    if (mapping) CloseHandle(mapping);
    return timed_out || remaining ? 2 : 0;
}
