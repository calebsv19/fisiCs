#include <stdio.h>
#define OSP3_DEFAULT_SEED 0x126ef4a9u
#include "15__probe_osp3_policy_matrix_common.h"

typedef uint32_t (*policy_fn)(uint32_t, uint32_t);

static uint32_t add_policy(uint32_t value, uint32_t key) {
    return value + key;
}

static uint32_t xor_policy(uint32_t value, uint32_t key) {
    return value ^ key;
}

static uint32_t rotate_policy(uint32_t value, uint32_t key) {
    uint32_t shift = key & 31u;
    if (shift == 0u) return value;
    return (value << shift) | (value >> (32u - shift));
}

int main(void) {
    policy_fn table[3] = {add_policy, xor_policy, rotate_policy};
    uint32_t seed = OSP3_SEED;
    uint32_t hash = 0xd2511f53u;
    uint32_t i;
    for (i = 0u; i < OSP3_CASE_BUDGET; ++i) {
        uint32_t value = osp3_next(&seed);
        uint32_t key = osp3_next(&seed);
        uint32_t lane = (value ^ key) % 3u;
        hash = osp3_mix(hash, table[lane](value, key));
        hash = osp3_mix(hash, lane);
    }
    printf("OSP3 callback seed=%08x cases=%u digest=%u\n",
           (unsigned)OSP3_SEED, (unsigned)OSP3_CASE_BUDGET, (unsigned)hash);
    return 0;
}
