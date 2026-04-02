#include <stdio.h>

int main(void) {
    volatile char values[4] = {(char)-1, (char)127, (char)128, (char)255};
    unsigned long long acc = 0ULL;
    int neg_mask = 0;
    int i = 0;

    for (i = 0; i < 4; ++i) {
        int promoted = values[i];
        acc = (acc * 65599ULL) + (unsigned int)(promoted & 0xFFFF);
        if (promoted < 0) {
            neg_mask |= 1 << i;
        }
    }

    printf("%llu %d\n", acc, neg_mask);
    return 0;
}
