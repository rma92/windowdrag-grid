#ifndef WG_CORE_H
#define WG_CORE_H
typedef struct { long left, top, right, bottom; } WgRect;
typedef struct { int columns, rows; } WgGrid;
typedef struct {
    int active, anchoring, column, row;
    WgRect original, area, selection;
    WgGrid grid;
} WgGesture;
long wg_boundary(long start, long end, int index, int count);
int wg_cell(long position, long start, long end, int count);
int wg_grid_valid(WgGrid grid);
void wg_begin(WgGesture *g, WgGrid grid, WgRect original, WgRect area, long x, long y);
void wg_update(WgGesture *g, WgRect area, long x, long y, int right);
WgRect wg_finish(WgGesture *g, int cancel);
#endif
