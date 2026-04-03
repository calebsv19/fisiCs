#include <stdio.h>

static long long mix_width(
    signed char a,
    unsigned char b,
    short c,
    unsigned short d,
    int e,
    unsigned int f,
    long long g) {
    return (long long)a +
           2LL * (long long)b +
           3LL * (long long)c +
           4LL * (long long)d +
           5LL * (long long)e +
           6LL * (long long)f +
           7LL * g;
}

int main(void) {
    long long value = mix_width(-3, 4, -5, 6, -7, 8U, 9LL);
    printf("%lld\n", value);
    return 0;
}
