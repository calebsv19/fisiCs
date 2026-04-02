#include <stdio.h>

int main(void) {
    volatile int seeds[4] = {-1, -7, -129, -4097};
    int width = (int)(sizeof(int) * 8u);
    int shifts[4];
    unsigned long long acc = 0ULL;
    int neg_mask = 0;
    int i;
    int j;

    shifts[0] = 0;
    shifts[1] = 1;
    shifts[2] = width / 2;
    shifts[3] = width - 1;

    for (i = 0; i < 4; ++i) {
        for (j = 0; j < 4; ++j) {
            int r = seeds[i] >> shifts[j];
            acc = (acc * 1469598103934665603ULL) ^ (unsigned int)r;
            if (r < 0) {
                neg_mask |= 1 << (i * 4 + j);
            }
        }
    }

    printf("%llu %d %d\n", acc, neg_mask, width);
    return 0;
}
