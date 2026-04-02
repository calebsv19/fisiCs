#include <stdio.h>

struct Block {
    unsigned char tag;
    unsigned short lane;
    unsigned value;
    unsigned long long stamp;
};

unsigned mt27_layout_digest(const struct Block *arr, unsigned count);
unsigned mt27_layout_stride_score(const struct Block *arr, unsigned count);

int main(void) {
    enum { ROWS = 28, COLS = 19, COUNT = ROWS * COLS };
    struct Block grid[ROWS][COLS];
    unsigned seed = 0x2468ace1u;

    for (unsigned r = 0u; r < ROWS; ++r) {
        for (unsigned c = 0u; c < COLS; ++c) {
            struct Block *x = &grid[r][c];
            seed = seed * 1103515245u + 12345u + r * 17u + c * 31u;
            x->tag = (unsigned char)((seed ^ (r * 3u + c * 5u)) & 0xffu);
            x->lane = (unsigned short)((seed >> 3u) & 0xffffu);
            x->value = seed ^ (r * 101u + c * 29u);
            x->stamp = ((unsigned long long)seed << 32u) | (unsigned long long)(r * 257u + c * 131u);
        }
    }

    printf(
        "%u %u\n",
        mt27_layout_digest(&grid[0][0], COUNT),
        mt27_layout_stride_score(&grid[0][0], COUNT)
    );
    return 0;
}
