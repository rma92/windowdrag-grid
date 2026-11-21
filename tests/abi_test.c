#include "../src/core.h"
#include <stdio.h>
static WgGesture gesture;
static void place(WgRect r) { printf("%ld\n",r.left); }
int main(void) { gesture.selection.left=123; place(gesture.selection); return 0; }
