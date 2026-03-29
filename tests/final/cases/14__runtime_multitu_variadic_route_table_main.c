#include <stdio.h>

long long abi_variadic_route(int tag, int lanes, ...);
long long abi_variadic_driver(int seed);

int main(void) {
    long long a = abi_variadic_route(
        2,
        6,
        0, -5,
        1, 22u,
        2, 1.5,
        3, 9000000005LL,
        0, 11,
        2, -0.25
    );
    long long b = abi_variadic_driver(4);
    printf("%lld %lld\n", a, b);
    return 0;
}
