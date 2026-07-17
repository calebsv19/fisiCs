#include <stdio.h>

struct wave81_payload {
    long a;
    long b;
    long c;
};

struct wave81_result {
    long x;
    long y;
    long z;
    long w;
};

typedef long (*wave81_callback_fn)(struct wave81_payload payload, long salt);

struct wave81_bundle {
    wave81_callback_fn first;
    wave81_callback_fn second;
    long seed;
};

struct wave81_result wave81_dispatch(struct wave81_bundle bundle,
                                     struct wave81_payload payload);

static long wave81_first(struct wave81_payload payload, long salt) {
    return payload.a * 2 + payload.b * 3 + payload.c + salt;
}

static long wave81_second(struct wave81_payload payload, long salt) {
    return payload.a - payload.b + payload.c * 4 - salt;
}

int main(void) {
    struct wave81_payload payload = {2, 5, 7};
    struct wave81_bundle bundle = {wave81_first, wave81_second, 11};
    struct wave81_result result = wave81_dispatch(bundle, payload);
    printf("%ld %ld %ld %ld\n", result.x, result.y, result.z, result.w);
    return 0;
}
