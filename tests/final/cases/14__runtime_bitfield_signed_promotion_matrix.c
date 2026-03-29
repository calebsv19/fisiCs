#include <stdio.h>

struct SignedBits {
    signed int a : 6;
    signed int b : 6;
    unsigned int c : 5;
};

int main(void) {
    struct SignedBits x = {-7, 12, 17u};
    unsigned score = 0u;

    for (unsigned i = 0; i < 5u; ++i) {
        x.a = x.a + 1;
        x.b = x.b - 1;
        x.c = (x.c + 3u) & 31u;

        score = score * 53u
              + (unsigned)(x.a + 32)
              + (unsigned)(x.b + 32) * 64u
              + x.c * 4096u;
    }

    printf("%d %d %u %u\n", x.a, x.b, x.c, score);
    return 0;
}
