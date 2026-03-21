#include <stdio.h>

int main(void) {
    volatile int source = 255;
    signed char sc = (signed char)source;
    unsigned char uc = (unsigned char)source;

    printf("%d %d\n", (int)sc, (int)uc);
    return 0;
}
