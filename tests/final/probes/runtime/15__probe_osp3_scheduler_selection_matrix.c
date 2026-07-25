#include <stdio.h>
#define OSP3_DEFAULT_SEED 0x5ce4d219u
#include "15__probe_osp3_policy_matrix_common.h"

struct task {
    uint32_t state;
    uint32_t priority;
    uint32_t wake_tick;
};

static uint32_t select_task(const struct task *tasks, uint32_t count,
                            uint32_t now) {
    uint32_t selected = count;
    uint32_t best = 0xffffffffu;
    uint32_t i;
    for (i = 0u; i < count; ++i) {
        uint32_t ready = tasks[i].state == 1u ||
                         (tasks[i].state == 2u && tasks[i].wake_tick <= now);
        if (ready && tasks[i].priority < best) {
            selected = i;
            best = tasks[i].priority;
        }
    }
    return selected;
}

int main(void) {
    uint32_t seed = OSP3_SEED;
    uint32_t hash = 0xa341316cu;
    uint32_t case_index;
    for (case_index = 0u; case_index < OSP3_CASE_BUDGET; ++case_index) {
        struct task tasks[4];
        uint32_t now = osp3_next(&seed) & 255u;
        uint32_t i;
        for (i = 0u; i < 4u; ++i) {
            uint32_t bits = osp3_next(&seed);
            tasks[i].state = bits & 3u;
            tasks[i].priority = (bits >> 2) & 31u;
            tasks[i].wake_tick = (bits >> 7) & 255u;
        }
        hash = osp3_mix(hash, select_task(tasks, 4u, now) | (now << 8));
    }
    printf("OSP3 scheduler seed=%08x cases=%u digest=%u\n",
           (unsigned)OSP3_SEED, (unsigned)OSP3_CASE_BUDGET, (unsigned)hash);
    return 0;
}
