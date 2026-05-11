#include <stdio.h>

static const int* g_vals = (int[]){1, 2};
static int g_cells[2];

static void init_cells(void) {
    g_cells[0] = g_vals[0];
    g_cells[1] = g_vals[1];
}

int main(void) {
    init_cells();
    printf("%d %d\n", g_cells[0], g_cells[1]);
    return 0;
}
