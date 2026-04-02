#include <stddef.h>
#include <stdio.h>

typedef union {
    unsigned u;
    unsigned short pair[2];
} Payload;

typedef struct {
    unsigned char tag;
    unsigned short lane;
    Payload payload;
    int bias;
} Node;

static unsigned mix32(unsigned x) {
    x ^= x >> 16u;
    x *= 0x7feb352du;
    x ^= x >> 15u;
    x *= 0x846ca68bu;
    x ^= x >> 16u;
    return x;
}

static unsigned digest_nodes(Node *arr, unsigned n, unsigned seed) {
    unsigned acc = 0x811c9dc5u;

    for (unsigned i = 0u; i < n; ++i) {
        seed = seed * 1664525u + 1013904223u + i * 29u;
        arr[i].tag = (unsigned char)(seed & 0xffu);
        arr[i].lane = (unsigned short)((seed >> 8u) & 0xffffu);
        arr[i].payload.u = seed ^ (i * 131u + 17u);
        arr[i].bias = (int)((seed >> 19u) & 0x1ffu) - (int)(i * 7u);
    }

    for (unsigned i = 0u; i < n; ++i) {
        unsigned pair_fold =
            (unsigned)arr[i].payload.pair[0] * 3u +
            (unsigned)arr[i].payload.pair[1] * 5u;
        unsigned lane =
            (unsigned)arr[i].tag * 11u +
            (unsigned)arr[i].lane * 13u +
            arr[i].payload.u ^
            pair_fold ^
            (unsigned)arr[i].bias;
        acc = (acc ^ mix32(lane + i * 17u)) * 16777619u;
    }

    {
        unsigned stride = (unsigned)((const char *)(arr + 1) - (const char *)arr);
        unsigned off_lane = (unsigned)offsetof(Node, lane);
        unsigned off_payload = (unsigned)offsetof(Node, payload);
        unsigned off_bias = (unsigned)offsetof(Node, bias);
        acc ^= stride * 19u + off_lane * 23u + off_payload * 29u + off_bias * 31u;
    }

    return acc ^ (acc >> 11u);
}

int main(void) {
    Node buf[33];
    unsigned a = digest_nodes(buf, 33u, 0x13579bdfu);
    unsigned b = digest_nodes(buf, 27u, 0x2468ace0u);
    printf("%u %u\n", a, b ^ (a * 7u + 19u));
    return 0;
}
