#include <stdio.h>

int main(void) {
    signed char sc = -3;
    unsigned char uc = 250u;
    float fa = 1.5f;
    float fb = -2.25f;

    printf("%d %u %.2f %.2f\n", sc, uc, fa, fb);
    return 0;
}
