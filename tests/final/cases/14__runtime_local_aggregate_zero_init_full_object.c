#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef struct AggregateProbe {
    uint64_t a;
    void* p;
    int arr[17];
    unsigned char bytes[19];
    double d;
} AggregateProbe;

static int probe_once(int seed) {
    volatile unsigned char poison[1024];
    for (int i = 0; i < 1024; ++i) {
        poison[i] = (unsigned char)(seed + i);
    }

    AggregateProbe probe = {0};
    unsigned char expected[sizeof(AggregateProbe)];
    memset(expected, 0, sizeof(expected));

    if (memcmp(&probe, expected, sizeof(AggregateProbe)) != 0) {
        return 1;
    }
    if (probe.p != NULL || probe.a != 0u || probe.d != 0.0) {
        return 2;
    }
    return 0;
}

int main(void) {
    for (int i = 0; i < 256; ++i) {
        int rc = probe_once(i);
        if (rc != 0) {
            printf("MISMATCH %d\n", rc);
            return 1;
        }
    }
    puts("OK");
    return 0;
}
