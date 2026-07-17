#include <stdio.h>

union AbiPayloadBits {
    unsigned word[3];
    struct {
        unsigned lo;
        unsigned hi;
        unsigned mask;
    } named;
};

struct AbiPayloadLeaf {
    unsigned tag;
    int delta;
    long double impulse;
    union AbiPayloadBits bits;
};

struct AbiPayloadFrame {
    unsigned epoch;
    struct AbiPayloadLeaf lanes[4];
    union {
        struct {
            int slot;
            unsigned salt;
        } route;
        unsigned raw[2];
    } meta;
    long double scale;
};

struct AbiPayloadFrame abi_payload_make_frame(unsigned seed, int flip);
struct AbiPayloadFrame abi_payload_relay_frame(struct AbiPayloadFrame in, unsigned step);
struct AbiPayloadFrame abi_payload_permute_frame(struct AbiPayloadFrame in, unsigned step);
unsigned abi_payload_fold_frame(struct AbiPayloadFrame frame, unsigned salt);

typedef struct AbiPayloadFrame (*AbiPayloadStepFn)(struct AbiPayloadFrame, unsigned);

static unsigned rotl32(unsigned x, unsigned n) {
    return (x << n) | (x >> (32u - n));
}

int main(void) {
    AbiPayloadStepFn steps[2];
    struct AbiPayloadFrame ring[3];
    unsigned acc = 0x811c9dc5u;
    unsigned i;

    steps[0] = abi_payload_relay_frame;
    steps[1] = abi_payload_permute_frame;

    ring[0] = abi_payload_make_frame(37u, -3);
    ring[1] = abi_payload_make_frame(91u, 5);
    ring[2] = abi_payload_make_frame(143u, -7);

    for (i = 0u; i < 11u; ++i) {
        unsigned src = (i + ring[i % 3u].meta.raw[0]) % 3u;
        unsigned dst = (src + 1u + (i & 1u)) % 3u;
        struct AbiPayloadFrame next = steps[i & 1u](ring[src], i + 13u);
        ring[dst] = next;
        acc ^= abi_payload_fold_frame(next, rotl32(i * 97u + 29u, (i & 7u) + 3u));
        acc = rotl32(acc + ring[dst].lanes[(i + dst) & 3u].bits.named.mask, 9u);
    }

    printf("%u %u\n", acc, abi_payload_fold_frame(ring[1], 0x6d2b79f5u));
    return 0;
}
