#include <stdio.h>

typedef struct {
    unsigned a;
    unsigned b;
    unsigned c;
    unsigned d;
    unsigned e;
    unsigned f;
    unsigned g;
} Cell7;

static unsigned rotl(unsigned v, unsigned s) {
    s &= 31u;
    if (s == 0u) {
        return v;
    }
    return (v << s) | (v >> (32u - s));
}

int main(void) {
    enum { ROWS = 72, COLS = 60 };
    Cell7 grid[ROWS][COLS];
    unsigned seed = 0x13579bdfu;
    unsigned acc = 2166136261u;

    for (unsigned r = 0; r < ROWS; ++r) {
        for (unsigned c = 0; c < COLS; ++c) {
            seed = seed * 1664525u + 1013904223u + r * 17u + c * 19u;
            grid[r][c].a = seed ^ (r * 41u + c * 5u);
            grid[r][c].b = rotl(seed + r * 11u, c & 15u);
            grid[r][c].c = grid[r][c].a + (grid[r][c].b >> 3u);
            grid[r][c].d = grid[r][c].a ^ grid[r][c].b ^ (r * 13u + c * 7u);
            grid[r][c].e = grid[r][c].c + grid[r][c].d + r + c;
            grid[r][c].f = rotl(grid[r][c].e ^ grid[r][c].a, (r + c) & 7u);
            grid[r][c].g = (grid[r][c].f + grid[r][c].b) ^ (r * 29u + c * 31u);
        }
    }

    for (unsigned r = 0; r < ROWS; ++r) {
        for (unsigned c = 0; c < COLS; ++c) {
            const Cell7 *x = &grid[r][c];
            unsigned lane =
                x->a + 3u * x->b + 5u * x->c + 7u * x->d + 11u * x->e + 13u * x->f + 17u * x->g;
            acc ^= rotl(lane ^ (r * 97u + c * 193u), (r + c) & 7u);
            acc *= 16777619u;
        }
    }

    printf("%u\n", acc);
    return 0;
}
