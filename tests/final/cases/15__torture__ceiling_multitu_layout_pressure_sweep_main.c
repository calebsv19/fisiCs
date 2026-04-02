#include <stdio.h>

struct LayoutNode {
    unsigned char tag;
    unsigned short lane;
    unsigned value;
    unsigned long long stamp;
    int bias;
};

unsigned mt36_layout_digest(const struct LayoutNode *arr, unsigned count);
unsigned mt36_layout_stride_score(const struct LayoutNode *arr, unsigned count);
unsigned mt36_layout_cross_fold(const struct LayoutNode *arr, unsigned count);

int main(void) {
    enum { ROWS = 26, COLS = 23, COUNT = ROWS * COLS };
    struct LayoutNode grid[ROWS][COLS];
    unsigned seed = 0x7A5D3C19u;

    for (unsigned r = 0u; r < ROWS; ++r) {
        for (unsigned c = 0u; c < COLS; ++c) {
            struct LayoutNode *x = &grid[r][c];
            seed = seed * 1103515245u + 12345u + r * 37u + c * 19u;
            x->tag = (unsigned char)((seed ^ (r * 5u + c * 3u)) & 0xffu);
            x->lane = (unsigned short)((seed >> 4u) & 0xffffu);
            x->value = seed ^ (r * 257u + c * 97u);
            x->stamp = ((unsigned long long)seed << 32u) | (unsigned long long)(r * 409u + c * 131u);
            x->bias = (int)((r * 7u) ^ (c * 11u)) - (int)(seed & 31u);
        }
    }

    printf(
        "%u %u\n",
        mt36_layout_digest(&grid[0][0], COUNT),
        mt36_layout_stride_score(&grid[0][0], COUNT) ^ mt36_layout_cross_fold(&grid[0][0], COUNT)
    );
    return 0;
}
