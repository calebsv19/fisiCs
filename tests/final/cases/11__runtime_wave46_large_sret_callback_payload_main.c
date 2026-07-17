#include <stdio.h>

typedef struct Payload46C {
    int a;
    int b;
    int c;
} Payload46C;

typedef struct Small46C {
    unsigned long long x;
    unsigned long long y;
} Small46C;

typedef struct Large46C {
    unsigned long long slots[5];
} Large46C;

typedef Small46C (*Callback46C)(Payload46C, int);

Large46C wave46_large_sret_callback_payload(Callback46C callback, Payload46C payload, int route);

static Small46C callback_payload(Payload46C payload, int route) {
    Small46C small;
    small.x = (unsigned long long)(payload.a + payload.b + route);
    small.y = (unsigned long long)(payload.b * payload.c + route);
    return small;
}

int main(void) {
    Payload46C payload = {4, 9, 3};
    Large46C large = wave46_large_sret_callback_payload(callback_payload, payload, 5);
    unsigned long long checksum = large.slots[0] + large.slots[1] + large.slots[2] + large.slots[3] + large.slots[4];
    if (large.slots[0] != 18ULL) return 1;
    if (large.slots[4] != 57ULL) return 2;
    if (checksum != 167ULL) return 3;
    printf("%llu %llu %llu\n", checksum, large.slots[2], large.slots[4]);
    return 0;
}
