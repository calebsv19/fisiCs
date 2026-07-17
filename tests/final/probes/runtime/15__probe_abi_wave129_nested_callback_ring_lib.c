#include "15__probe_abi_wave129_nested_callback_ring.h"

static unsigned rotl32(unsigned value, unsigned shift) {
    shift &= 31u;
    if (shift == 0u) {
        return value;
    }
    return (value << shift) | (value >> (32u - shift));
}

struct Wave129RingPayload wave129_ring_seed(unsigned seed, long bias) {
    struct Wave129RingPayload payload;
    unsigned row;
    unsigned col;

    payload.epoch = seed ^ rotl32((unsigned)(bias * 23L + 97L), seed & 31u);
    for (row = 0u; row < 2u; ++row) {
        for (col = 0u; col < 3u; ++col) {
            unsigned base = seed + row * 71u + col * 113u + (unsigned)(bias * 5L);
            payload.leaves[row][col].bias = bias + (long)(row * 17u) - (long)(col * 19u);
            payload.leaves[row][col].bits.named.lo = base * 13u + 11u;
            payload.leaves[row][col].bits.named.hi = rotl32(base ^ 0x7f4a7c15u, row + col + 5u);
            payload.leaves[row][col].bits.named.tag =
                payload.leaves[row][col].bits.named.lo ^
                payload.leaves[row][col].bits.named.hi ^
                (unsigned)(payload.leaves[row][col].bias * 29L);
        }
    }
    return payload;
}

struct Wave129RingPayload wave129_ring_step(struct Wave129RingPayload payload,
                                            Wave129RingCallback callback,
                                            unsigned step) {
    unsigned row;
    unsigned col;

    payload = callback(payload, step);
    payload.epoch ^= rotl32(step + payload.leaves[step & 1u][step % 3u].bits.named.tag, step + 7u);

    for (row = 0u; row < 2u; ++row) {
        for (col = 0u; col < 3u; ++col) {
            struct Wave129RingLeaf leaf = payload.leaves[row][col];
            leaf.bias += (long)((int)((step + row + col) & 7u) - 3);
            leaf.bits.words[(row + col) % 3u] ^= leaf.bits.named.tag + step + (unsigned)leaf.bias;
            payload.leaves[(row + step) & 1u][(col + step) % 3u] = leaf;
        }
    }

    return payload;
}

unsigned wave129_ring_fold(struct Wave129RingPayload payload, unsigned salt) {
    unsigned acc = salt ^ payload.epoch;
    unsigned row;
    unsigned col;

    for (row = 0u; row < 2u; ++row) {
        for (col = 0u; col < 3u; ++col) {
            struct Wave129RingLeaf leaf = payload.leaves[row][col];
            acc ^= leaf.bits.named.lo + leaf.bits.named.hi + leaf.bits.named.tag;
            acc = rotl32(acc + (unsigned)(leaf.bias * 131L), row * 11u + col + 3u);
        }
    }

    return acc ^ (acc >> 15u);
}
