#include <stdio.h>

typedef struct Seed47C {
    int base;
    int delta;
} Seed47C;

typedef struct Payload47C {
    int values[3];
    int tag;
} Payload47C;

typedef struct Result47C {
    Payload47C payloads[2];
    int marker;
} Result47C;

typedef Payload47C (*PayloadCallback47C)(Seed47C, int);

Result47C wave47_callback_returned_payload_chain(PayloadCallback47C callback, Seed47C seed, int route);

static Payload47C build_payload47(Seed47C seed, int route) {
    Payload47C payload;
    payload.values[0] = seed.base + route;
    payload.values[1] = seed.delta * route;
    payload.values[2] = seed.base + seed.delta + route;
    payload.tag = route - seed.base;
    return payload;
}

int main(void) {
    Seed47C seed = {4, 6};
    Result47C result = wave47_callback_returned_payload_chain(build_payload47, seed, 5);
    int checksum = result.payloads[0].values[0] + result.payloads[0].values[1] +
        result.payloads[0].values[2] + result.payloads[0].tag +
        result.payloads[1].values[0] + result.payloads[1].values[1] +
        result.payloads[1].values[2] + result.payloads[1].tag + result.marker;
    if (result.payloads[0].values[1] != 30) return 1;
    if (result.payloads[1].values[2] != 20) return 2;
    if (checksum != 145) return 3;
    printf("%d %d %d\n", checksum, result.payloads[1].values[2], result.marker);
    return 0;
}
