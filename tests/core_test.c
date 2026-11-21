#include <assert.h>
#include <stdio.h>
#include "../src/core.h"
int main(void)
{
    WgGrid grid; WgGesture g; WgRect area, original, result;
    int count, i; long a, b;
    area.left=-1920; area.top=-200; area.right=1; area.bottom=881;
    original.left=10; original.top=20; original.right=400; original.bottom=300;
    for (count=1;count<=64;++count) {
        assert(wg_boundary(-1920,1,0,count)==-1920);
        assert(wg_boundary(-1920,1,count,count)==1);
        for (i=0;i<count;++i) {
            a=wg_boundary(-1920,1,i,count); b=wg_boundary(-1920,1,i+1,count);
            assert(b>a); assert(wg_cell(a,-1920,1,count)==i);
            assert(wg_cell(b-1,-1920,1,count)==i);
        }
    }
    grid.columns=12; grid.rows=8;
    /* Cell membership changes at the boundary, never at its midpoint. */
    assert(wg_cell(49,0,400,4)==0);
    assert(wg_cell(99,0,400,4)==0);
    assert(wg_cell(100,0,400,4)==1);
    assert(wg_cell(101,0,400,4)==1);
    wg_begin(&g,grid,original,area,0,880);
    wg_update(&g,area,0,880,0);
    wg_update(&g,area,-1920,-200,0);
    assert(g.selection.left==area.left && g.selection.top==area.top);
    assert(g.selection.right==area.right && g.selection.bottom==area.bottom);
    area.left=100; area.right=1300;
    wg_update(&g,area,100,0,0); assert(g.area.left==-1920);
    wg_update(&g,area,100,0,1); assert(g.area.left==100);
    wg_update(&g,area,100,0,0);
    assert(g.selection.right-g.selection.left==100);
    result=wg_finish(&g,1); assert(!g.active && result.left==original.left);
    grid.rows=65; wg_begin(&g,grid,original,area,100,0); assert(!g.active);
    grid.rows=1; grid.columns=1;
    wg_begin(&g,grid,original,area,-99999,99999);
    result=wg_finish(&g,0); assert(result.left==area.left && result.bottom==area.bottom);
    puts("core tests passed"); return 0;
}
