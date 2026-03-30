#include <stdio.h>

typedef struct {
    unsigned a;
    unsigned b;
    unsigned c;
    unsigned d;
} Cell;

static unsigned rotl(unsigned v, unsigned s) {
    s &= 31u;
    if (s == 0u) {
        return v;
    }
    return (v << s) | (v >> (32u - s));
}

int main(void) {
    enum { ROWS = 48, COLS = 36 };
    Cell grid[ROWS][COLS];
    unsigned seed = 0x10203040u;
    unsigned acc = 2166136261u;

    for (unsigned r = 0; r < ROWS; ++r) {
        for (unsigned c = 0; c < COLS; ++c) {
            seed = seed * 1103515245u + 12345u + r * 7u + c * 13u;
            grid[r][c].a = seed ^ (r * 31u + c);
            grid[r][c].b = rotl(seed + r * 17u, c & 15u);
            grid[r][c].c = (grid[r][c].a & 0xffffu) + (grid[r][c].b >> 3u);
            grid[r][c].d = grid[r][c].a ^ grid[r][c].b ^ grid[r][c].c;
        }
    }

    for (unsigned r = 0; r < ROWS; ++r) {
        for (unsigned c = 0; c < COLS; ++c) {
            const Cell *x = &grid[r][c];
            unsigned lane = x->a + 3u * x->b + 5u * x->c + 7u * x->d;
            acc ^= rotl(lane ^ (r * 97u + c * 193u), (r + c) & 7u);
            acc *= 16777619u;
        }
    }

    printf("%u\n", acc);
    return 0;
}
