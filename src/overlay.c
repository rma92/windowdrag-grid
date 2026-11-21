#include "win.h"
static HWND overlay;
static WgGesture view;
static WgSettings style;
static LRESULT CALLBACK overlay_proc(HWND w,UINT msg,WPARAM wp,LPARAM lp)
{
    PAINTSTRUCT ps; HDC dc; HPEN pen,old; int i,x,y;
    if (msg==WM_NCHITTEST) return HTTRANSPARENT;
    if (msg==WM_ERASEBKGND) return 1;
    if (msg==WM_PAINT) {
        dc=BeginPaint(w,&ps);
        pen=CreatePen(PS_SOLID,style.thickness,style.grid_color);
        old=(HPEN)SelectObject(dc,pen);
        for (i=0;i<=view.grid.columns;++i) {
            x=(int)(wg_boundary(view.area.left,view.area.right,i,view.grid.columns)-view.area.left);
            if (i==view.grid.columns) --x;
            MoveToEx(dc,x,0,NULL); LineTo(dc,x,(int)(view.area.bottom-view.area.top));
        }
        for (i=0;i<=view.grid.rows;++i) {
            y=(int)(wg_boundary(view.area.top,view.area.bottom,i,view.grid.rows)-view.area.top);
            if (i==view.grid.rows) --y;
            MoveToEx(dc,0,y,NULL); LineTo(dc,(int)(view.area.right-view.area.left),y);
        }
        SelectObject(dc,old); DeleteObject(pen);
        pen=CreatePen(PS_SOLID,style.thickness+1,style.selection_color);
        old=(HPEN)SelectObject(dc,pen);
        SelectObject(dc,GetStockObject(NULL_BRUSH));
        Rectangle(dc,(int)(view.selection.left-view.area.left),(int)(view.selection.top-view.area.top),
            (int)(view.selection.right-view.area.left-1),(int)(view.selection.bottom-view.area.top-1));
        SelectObject(dc,old); DeleteObject(pen); EndPaint(w,&ps); return 0;
    }
    return DefWindowProcA(w,msg,wp,lp);
}
static void band(HRGN region,int l,int t,int r,int b)
{
    HRGN part; part=CreateRectRgn(l,t,r,b); CombineRgn(region,region,part,RGN_OR); DeleteObject(part);
}
void wg_overlay_hide(void) { if (overlay) ShowWindow(overlay,SW_HIDE); }
void wg_overlay_show(const WgGesture *g,const WgSettings *s)
{
    WNDCLASSA wc; HRGN region; int i,x,y,width,height,n,l,t,r,b;
    if (!g->active) { wg_overlay_hide(); return; }
    if (!overlay) {
        ZeroMemory(&wc,sizeof(wc)); wc.hInstance=GetModuleHandleA(NULL);
        wc.lpfnWndProc=overlay_proc; wc.lpszClassName="WindowDragGrid.Overlay.v1";
        RegisterClassA(&wc);
        overlay=CreateWindowExA(WS_EX_TOOLWINDOW|WS_EX_TRANSPARENT,"WindowDragGrid.Overlay.v1","",
            WS_POPUP,0,0,0,0,NULL,NULL,wc.hInstance,NULL);
        if (!overlay) return;
    }
    view=*g; style=*s; n=s->thickness+1;
    width=(int)(g->area.right-g->area.left); height=(int)(g->area.bottom-g->area.top);
    region=CreateRectRgn(0,0,0,0);
    for (i=0;i<=g->grid.columns;++i) {
        x=(int)(wg_boundary(0,width,i,g->grid.columns)); if (i==g->grid.columns) --x;
        band(region,x-n,0,x+n,height);
    }
    for (i=0;i<=g->grid.rows;++i) {
        y=(int)(wg_boundary(0,height,i,g->grid.rows)); if (i==g->grid.rows) --y;
        band(region,0,y-n,width,y+n);
    }
    l=(int)(g->selection.left-g->area.left); t=(int)(g->selection.top-g->area.top);
    r=(int)(g->selection.right-g->area.left-1); b=(int)(g->selection.bottom-g->area.top-1);
    band(region,l-n,t-n,r+n,t+n); band(region,l-n,b-n,r+n,b+n);
    band(region,l-n,t-n,l+n,b+n); band(region,r-n,t-n,r+n,b+n);
    if (!SetWindowRgn(overlay,region,TRUE)) DeleteObject(region);
    SetWindowPos(overlay,HWND_TOPMOST,(int)g->area.left,(int)g->area.top,width,height,
        SWP_NOACTIVATE|SWP_SHOWWINDOW);
    InvalidateRect(overlay,NULL,FALSE); UpdateWindow(overlay);
}
