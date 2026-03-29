#include <stdio.h>

long long abi_sum_tagged(const char* tag, int count, ...);

int main(void) {
    long long value = abi_sum_tagged("ok", 4, 3, 4000000000LL, 5, -2LL);
    printf("%lld\n", value);
    return 0;
}

