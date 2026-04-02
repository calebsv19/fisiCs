#include <stdio.h>

enum {
    ROWS = (1 << 5),
    COLS = (1 << 4) + 3,
    CELLS = (ROWS * COLS) - 7
};

static int g_buf[CELLS];

int main(void) {
    g_buf[0] = 9;
    g_buf[CELLS - 1] = 4;
    int count = (int)(sizeof(g_buf) / sizeof(g_buf[0]));
    printf("%d %d %d\n", count, g_buf[0], g_buf[CELLS - 1]);
    return 0;
}
