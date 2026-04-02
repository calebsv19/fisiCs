#include <stdio.h>

int main(void) {
    volatile unsigned char raw[6] = {0u, 1u, 127u, 128u, 165u, 255u};
    unsigned long long acc = 0ULL;
    int neg_mask = 0;
    int i;

    for (i = 0; i < 6; ++i) {
        char c = (char)raw[i];
        int promoted = c;
        acc = (acc * 1099511628211ULL) ^ (unsigned int)(promoted & 0xFFFF);
        if (promoted < 0) {
            neg_mask |= 1 << i;
        }
    }

    printf("%llu %d\n", acc, neg_mask);
    return 0;
}
