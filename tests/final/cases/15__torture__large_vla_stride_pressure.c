#include <stddef.h>
#include <stdio.h>

static unsigned long long vla_pressure(int rows, int cols) {
    unsigned long long grid[rows][cols];
    unsigned long long acc = 1469598103934665603ULL;

    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            unsigned long long base = (unsigned long long)(r + 1) * (unsigned long long)(c + 3);
            unsigned long long mix = (unsigned long long)((r ^ c) * 11u);
            grid[r][c] = base + mix + (unsigned long long)(r * cols + c);
        }
    }

    for (int r = 0; r < rows - 1; ++r) {
        unsigned long long *cur = grid[r];
        unsigned long long *nxt = grid[r + 1];
        ptrdiff_t delta = nxt - cur;
        if (delta != cols) {
            return 0ULL;
        }

        for (int c = 1; c < cols - 1; c += 3) {
            unsigned long long lane = cur[c - 1] + cur[c] * 3ULL + cur[c + 1] * 5ULL;
            unsigned long long cross = nxt[(c * 7) % cols] + grid[(r + 2) % rows][(c * 11) % cols];
            acc ^= lane + cross + (unsigned long long)(delta * (r + 1));
            acc = (acc << 9) | (acc >> 55);
            acc += 0x9e3779b97f4a7c15ULL;
        }
    }

    return acc ^ grid[rows - 1][cols - 1] ^ grid[rows / 2][cols / 2];
}

int main(void) {
    unsigned long long out = vla_pressure(73, 89);
    printf("%llu\n", out);
    return 0;
}
