#include <stdio.h>

typedef struct {
    unsigned a;
    unsigned b;
    unsigned c;
    unsigned d;
    unsigned e;
    unsigned f;
    unsigned g;
    unsigned h;
    unsigned long long stamp;
} Cell9;

static unsigned rotl32(unsigned v, unsigned s) {
    s &= 31u;
    if (s == 0u) {
        return v;
    }
    return (v << s) | (v >> (32u - s));
}

int main(void) {
    enum { ROWS = 68, COLS = 54 };
    Cell9 grid[ROWS][COLS];
    unsigned seed = 0xA5C31D2Fu;
    unsigned acc = 2166136261u;

    for (unsigned r = 0u; r < ROWS; ++r) {
        for (unsigned c = 0u; c < COLS; ++c) {
            Cell9 *x = &grid[r][c];
            seed = seed * 1664525u + 1013904223u + r * 13u + c * 29u;
            x->a = seed ^ (r * 17u + c * 5u);
            x->b = rotl32(seed + r * 7u, c & 15u);
            x->c = x->a + x->b + r;
            x->d = x->a ^ x->b ^ (c * 31u);
            x->e = x->c + x->d + r * 3u;
            x->f = rotl32(x->e ^ x->b, (r + c) & 7u);
            x->g = x->f + (x->d >> 1u);
            x->h = x->g ^ (r * 101u + c * 211u);
            x->stamp = ((unsigned long long)seed << 32u) | (unsigned long long)(r * 257u + c * 131u);
        }
    }

    for (unsigned r = 0u; r < ROWS; ++r) {
        for (unsigned c = 0u; c < COLS; ++c) {
            const Cell9 *x = &grid[r][c];
            unsigned lane =
                x->a + 3u * x->b + 5u * x->c + 7u * x->d +
                11u * x->e + 13u * x->f + 17u * x->g + 19u * x->h;
            lane ^= (unsigned)(x->stamp >> ((r + c) & 7u));
            acc ^= rotl32(lane ^ (r * 97u + c * 193u), (r + c) & 7u);
            acc *= 16777619u;
        }
    }

    printf("%u %u\n", acc, seed);
    return 0;
}
