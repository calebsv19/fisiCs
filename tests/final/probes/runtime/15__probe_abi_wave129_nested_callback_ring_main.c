#include "15__probe_abi_wave129_nested_callback_ring.h"
#include <stdio.h>

static unsigned rotl32(unsigned value, unsigned shift) {
    shift &= 31u;
    if (shift == 0u) {
        return value;
    }
    return (value << shift) | (value >> (32u - shift));
}

static struct Wave129RingPayload twist_a(struct Wave129RingPayload payload, unsigned step) {
    payload.leaves[step & 1u][(step + 1u) % 3u].bits.named.tag ^= step * 2654435761u;
    payload.leaves[(step + 1u) & 1u][step % 3u].bias += (long)(step * 3u + 5u);
    return payload;
}

static struct Wave129RingPayload twist_b(struct Wave129RingPayload payload, unsigned step) {
    payload.leaves[(step + 1u) & 1u][(step + 2u) % 3u].bits.words[step % 3u] +=
        rotl32(payload.epoch + step, step + 9u);
    payload.epoch += step * 17u + payload.leaves[step & 1u][step % 3u].bits.named.lo;
    return payload;
}

int main(void) {
    Wave129RingCallback callbacks[2] = {twist_a, twist_b};
    struct Wave129RingPayload ring[3];
    unsigned acc = 0x1234567u;
    unsigned i;

    ring[0] = wave129_ring_seed(23u, -7L);
    ring[1] = wave129_ring_seed(61u, 13L);
    ring[2] = wave129_ring_seed(89u, -19L);

    for (i = 0u; i < 10u; ++i) {
        unsigned src = (i + ring[i % 3u].epoch) % 3u;
        unsigned dst = (src + 1u + (ring[src].leaves[i & 1u][i % 3u].bits.named.tag & 1u)) % 3u;
        ring[dst] = wave129_ring_step(ring[src], callbacks[i & 1u], i + 5u);
        acc ^= wave129_ring_fold(ring[dst], rotl32(i * 199u + 31u, i + 4u));
        acc = rotl32(acc + ring[dst].epoch, 6u);
    }

    printf("%u %u %u\n",
           acc,
           wave129_ring_fold(ring[0], 0x7f4a7c15u),
           wave129_ring_fold(ring[2], 0x45d9f3bu));
    return 0;
}
