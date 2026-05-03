#include <stdio.h>

static int grid[16] = {
    [0] = 3,
    [5] = 8,
    [9] = 13,
    [15] = 21,
};

int main(void) {
    int checksum = 0;
    int zero_count = 0;
    int i = 0;
    for (i = 0; i < 16; ++i) {
        checksum += grid[i] * (i + 1);
        if (grid[i] == 0) {
            ++zero_count;
        }
    }
    printf("%d %d %d %d %d %d\n",
           checksum,
           zero_count,
           grid[0],
           grid[5],
           grid[9],
           grid[15]);
    return 0;
}
