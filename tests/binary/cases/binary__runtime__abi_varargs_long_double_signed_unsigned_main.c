#include <stdio.h>

long long w17_mix_variadic(const char *fmt, ...);

int main(void) {
    printf("%lld\n", w17_mix_variadic("LiuL", 1.25L, -3, 5u, 2.5L));
    return 0;
}
