#include <stdio.h>

long long w16_mix_variadic(const char *fmt, ...);

int main(void) {
    float f = 1.25f;
    printf("%lld\n", w16_mix_variadic("iulUd", -3, 7u, -5ll, 9ull, f));
    return 0;
}
