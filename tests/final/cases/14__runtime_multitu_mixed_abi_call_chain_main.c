#include <stdio.h>

struct MixPayload {
    long long big;
    int small;
};

long long abi_mix_bridge(
    int a,
    unsigned b,
    long long c,
    double d,
    struct MixPayload payload,
    unsigned short u
);

int main(void) {
    struct MixPayload payload;
    payload.big = 90LL;
    payload.small = 7;
    long long result = abi_mix_bridge(-5, 3U, 1000LL, 2.5, payload, 12U);
    printf("%lld\n", result);
    return 0;
}
