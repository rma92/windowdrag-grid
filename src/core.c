#include "core.h"
static int clamp(int n, int lo, int hi) { return n < lo ? lo : n > hi ? hi : n; }
int wg_grid_valid(WgGrid g) { return g.columns >= 1 && g.columns <= 64 && g.rows >= 1 && g.rows <= 64; }
long wg_boundary(long a, long b, int i, int n)
{
    long span;
    if (n < 1 || b <= a) return a;
    i = clamp(i, 0, n); span = b - a;
    return a + (span / n) * i + ((span % n) * i) / n;
}
int wg_cell(long p, long a, long b, int n)
{
    int lo, hi, mid;
    if (n < 1 || b <= a || p <= a) return 0;
    if (p >= b) return n - 1;
    lo = 0; hi = n;
    while (lo + 1 < hi) {
        mid = (lo + hi) / 2;
        if (wg_boundary(a, b, mid, n) <= p) lo = mid; else hi = mid;
    }
    return lo;
}
void wg_update(WgGesture *g, WgRect area, long x, long y, int right)
{
    int c, r, c0, c1, r0, r1;
    if (!g->active) return;
    if (right) { g->area = area; g->anchoring = 1; }
    c = wg_cell(x, g->area.left, g->area.right, g->grid.columns);
    r = wg_cell(y, g->area.top, g->area.bottom, g->grid.rows);
    if (g->anchoring) { g->column = c; g->row = r; }
    if (!right) g->anchoring = 0;
    c0 = c < g->column ? c : g->column; c1 = c > g->column ? c : g->column;
    r0 = r < g->row ? r : g->row; r1 = r > g->row ? r : g->row;
    g->selection.left = wg_boundary(g->area.left, g->area.right, c0, g->grid.columns);
    g->selection.right = wg_boundary(g->area.left, g->area.right, c1 + 1, g->grid.columns);
    g->selection.top = wg_boundary(g->area.top, g->area.bottom, r0, g->grid.rows);
    g->selection.bottom = wg_boundary(g->area.top, g->area.bottom, r1 + 1, g->grid.rows);
}
void wg_begin(WgGesture *g, WgGrid grid, WgRect original, WgRect area, long x, long y)
{
    g->active = wg_grid_valid(grid) && area.right > area.left && area.bottom > area.top;
    g->anchoring = 1; g->grid = grid; g->original = original; g->area = area;
    g->column = g->row = 0; g->selection = original;
    wg_update(g, area, x, y, 1);
}
WgRect wg_finish(WgGesture *g, int cancel)
{
    g->active = 0; return cancel ? g->original : g->selection;
}
