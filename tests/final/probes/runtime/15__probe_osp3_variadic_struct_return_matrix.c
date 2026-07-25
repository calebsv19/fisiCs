#include <stdarg.h>
#include <stdio.h>
#define OSP3_DEFAULT_SEED 0x8f7e6d5cu
#include "15__probe_osp3_policy_matrix_common.h"

struct packet {
    uint32_t first;
    uint32_t second;
    uint32_t third;
    uint32_t fourth;
};

static struct packet build_packet(uint32_t seed, uint32_t count, ...) {
    struct packet result;
    va_list args;
    uint32_t i;
    result.first = seed;
    result.second = 0u;
    result.third = 0x811c9dc5u;
    result.fourth = count;
    va_start(args, count);
    for (i = 0u; i < count; ++i) {
        uint32_t value = va_arg(args, uint32_t);
        result.first += value;
        result.second ^= value << (i & 7u);
        result.third = osp3_mix(result.third, value);
    }
    va_end(args);
    return result;
}

int main(void) {
    uint32_t seed = OSP3_SEED;
    uint32_t hash = 0x94d049bbu;
    uint32_t i;
    for (i = 0u; i < OSP3_CASE_BUDGET; ++i) {
        uint32_t a = osp3_next(&seed);
        uint32_t b = osp3_next(&seed);
        uint32_t c = osp3_next(&seed);
        uint32_t d = osp3_next(&seed);
        struct packet packet = build_packet(i, 4u, a, b, c, d);
        hash = osp3_mix(hash, packet.first);
        hash = osp3_mix(hash, packet.second);
        hash = osp3_mix(hash, packet.third);
        hash = osp3_mix(hash, packet.fourth);
    }
    printf("OSP3 variadic-sret seed=%08x cases=%u digest=%u\n",
           (unsigned)OSP3_SEED, (unsigned)OSP3_CASE_BUDGET, (unsigned)hash);
    return 0;
}
