#include <stdio.h>

typedef struct {
    unsigned a;
    unsigned b;
    unsigned c;
    unsigned d;
    unsigned e;
    unsigned f;
} Cell;

static unsigned rotl(unsigned v, unsigned s) {
    s &= 31u;
    if (s == 0u) {
        return v;
    }
    return (v << s) | (v >> (32u - s));
}

int main(void) {
    enum { ROWS = 64, COLS = 52 };
    Cell grid[ROWS][COLS];
    unsigned seed = 0x2468ace1u;
    unsigned acc = 2166136261u;

    for (unsigned r = 0; r < ROWS; ++r) {
        for (unsigned c = 0; c < COLS; ++c) {
            seed = seed * 1103515245u + 12345u + r * 11u + c * 17u;
            grid[r][c].a = seed ^ (r * 31u + c * 3u);
            grid[r][c].b = rotl(seed + r * 7u, c & 15u);
            grid[r][c].c = grid[r][c].a + (grid[r][c].b >> 3u);
            grid[r][c].d = grid[r][c].a ^ grid[r][c].b ^ (r * 13u + c * 5u);
            grid[r][c].e = grid[r][c].c + grid[r][c].d + (unsigned)(r + c);
            grid[r][c].f = rotl(grid[r][c].e ^ grid[r][c].a, (r + c) & 7u);
        }
    }

    for (unsigned r = 0; r < ROWS; ++r) {
        for (unsigned c = 0; c < COLS; ++c) {
            const Cell *x = &grid[r][c];
            unsigned lane = x->a + 3u * x->b + 5u * x->c + 7u * x->d + 11u * x->e + 13u * x->f;
            acc ^= rotl(lane ^ (r * 97u + c * 193u), (r + c) & 7u);
            acc *= 16777619u;
        }
    }

    printf("%u\n", acc);
    return 0;
}
