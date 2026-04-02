#include <stdio.h>

static unsigned run_vla(unsigned rows, unsigned cols, unsigned seed) {
    int m[rows][cols];
    unsigned acc = 0x811c9dc5u;

    for (unsigned r = 0u; r < rows; ++r) {
        for (unsigned c = 0u; c < cols; ++c) {
            seed = seed * 1103515245u + 12345u + r * 17u + c * 29u;
            m[r][c] = (int)((seed >> 9u) & 0x7ffu) - (int)(r * 11u + c * 7u);
        }
    }

    for (unsigned r = 0u; r + 1u < rows; ++r) {
        int (*row0)[cols] = &m[r];
        int (*row1)[cols] = &m[r + 1u];
        long diff = row1 - row0;
        acc ^= (unsigned)diff * 131u + r * 19u;
        acc *= 16777619u;
    }

    for (unsigned r = 0u; r < rows; ++r) {
        int (*rowp)[cols] = &m[r];
        for (unsigned c = 0u; c < cols; ++c) {
            int v = (*rowp)[c];
            int left = (c > 0u) ? (*rowp)[c - 1u] : v;
            int up = (r > 0u) ? m[r - 1u][c] : v;
            unsigned lane = (unsigned)(v ^ (left * 3) ^ (up * 5));
            acc ^= lane + r * 23u + c * 31u;
            acc = (acc << 7u) | (acc >> 25u);
        }
    }

    return acc ^ (acc >> 13u) ^ seed;
}

int main(void) {
    unsigned a = run_vla(17u, 19u, 0x13579bdfu);
    unsigned b = run_vla(11u, 23u, 0x2468ace0u);
    printf("%u %u\n", a, a ^ (b * 5u + 17u));
    return 0;
}
