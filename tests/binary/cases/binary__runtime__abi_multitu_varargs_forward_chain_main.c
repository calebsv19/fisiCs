#include <stdio.h>

long long w18_entry(const char *fmt, ...);

int main(void) {
    printf("%lld\n", w18_entry("iuLd", -2, 7u, 1.5L, 2.25));
    return 0;
}
