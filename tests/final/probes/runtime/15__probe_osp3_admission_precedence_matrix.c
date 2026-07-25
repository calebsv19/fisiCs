#include <stdio.h>
#define OSP3_DEFAULT_SEED 0x31a0d17bu
#include "15__probe_osp3_policy_matrix_common.h"

static uint32_t admit(uint32_t magic, uint32_t version, uint32_t cpu_request,
                      uint32_t memory_request, uint32_t cpu_available,
                      uint32_t memory_available) {
    if (magic != 0x4f535033u) return 1u;
    if (version != 3u) return 2u;
    if (cpu_request == 0u || memory_request == 0u) return 3u;
    if (cpu_request > cpu_available) return 4u;
    if (memory_request > memory_available) return 5u;
    return 0u;
}

int main(void) {
    uint32_t seed = OSP3_SEED;
    uint32_t hash = 0x811c9dc5u;
    uint32_t i;
    for (i = 0u; i < OSP3_CASE_BUDGET; ++i) {
        uint32_t bits = osp3_next(&seed);
        uint32_t magic = (bits & 1u) ? 0x4f535033u : bits;
        uint32_t version = (bits >> 1) & 7u;
        uint32_t cpu_available = 1u + ((bits >> 4) & 31u);
        uint32_t memory_available = 1u + ((bits >> 9) & 1023u);
        uint32_t cpu_request = (bits >> 19) & 63u;
        uint32_t memory_request = osp3_next(&seed) & 2047u;
        hash = osp3_mix(hash, admit(magic, version, cpu_request,
                                    memory_request, cpu_available,
                                    memory_available));
        hash = osp3_mix(hash, cpu_request ^ (memory_request << 1));
    }
    printf("OSP3 admission seed=%08x cases=%u digest=%u\n",
           (unsigned)OSP3_SEED, (unsigned)OSP3_CASE_BUDGET, (unsigned)hash);
    return 0;
}
