#include <stdio.h>

long long w18_deep_entry(int rounds, ...);

int main(void) {
    printf("%lld\n", w18_deep_entry(2, 1.25L, -2, 5u, 0.5L, 4, 3u));
    return 0;
}
