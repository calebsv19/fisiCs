#include <stdio.h>

int main(void) {
    volatile int seeds[4] = {-1, -17, -257, -1025};
    volatile int shifts[3] = {1, 3, 5};
    unsigned long long acc = 0ULL;
    int sign_bits = 0;
    int i = 0;
    int j = 0;

    for (i = 0; i < 4; ++i) {
        for (j = 0; j < 3; ++j) {
            int r = seeds[i] >> shifts[j];
            acc = (acc * 1315423911ULL) + (unsigned int)r;
            if (r < 0) {
                sign_bits |= 1 << (i * 3 + j);
            }
        }
    }

    printf("%llu %d\n", acc, sign_bits);
    return 0;
}
