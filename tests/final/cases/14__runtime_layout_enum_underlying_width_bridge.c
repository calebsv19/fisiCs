#include <stdio.h>

enum WidthMode {
    WM_ZERO = 0,
    WM_ONE = 1,
    WM_BIG = 2000000000
};

enum SignedMode {
    SM_NEG = -3,
    SM_ZERO = 0,
    SM_POS = 7
};

int main(void) {
    enum WidthMode w = WM_BIG;
    enum SignedMode s = SM_NEG;

    unsigned uw = (unsigned)w;
    int is = (int)s;
    long long mix = (long long)uw +
                    (long long)is * 13LL +
                    (long long)sizeof(enum WidthMode) * 101LL +
                    (long long)sizeof(enum SignedMode) * 17LL;

    printf("%zu %zu %u %d %lld\n",
           sizeof(enum WidthMode),
           sizeof(enum SignedMode),
           uw,
           is,
           mix);
    return 0;
}
