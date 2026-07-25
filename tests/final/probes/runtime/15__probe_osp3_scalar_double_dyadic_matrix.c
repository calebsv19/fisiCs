#include <stdio.h>
#define OSP3_DEFAULT_SEED 0xc3d2e1f0u
#include "15__probe_osp3_policy_matrix_common.h"

struct motion {
    double position;
    double velocity;
};

static struct motion integrate(double position, double velocity,
                               double acceleration, uint32_t steps) {
    struct motion result;
    uint32_t i;
    result.position = position;
    result.velocity = velocity;
    for (i = 0u; i < steps; ++i) {
        result.velocity += acceleration * 0.125;
        result.position += result.velocity * 0.125;
    }
    return result;
}

int main(void) {
    uint32_t seed = OSP3_SEED;
    uint32_t hash = 0x27d4eb2fu;
    uint32_t i;
    for (i = 0u; i < OSP3_CASE_BUDGET; ++i) {
        uint32_t bits = osp3_next(&seed);
        double position = (double)(bits & 255u) * 0.125;
        double velocity = (double)((bits >> 8) & 127u) * 0.125;
        double acceleration = (double)((bits >> 15) & 31u) * 0.125;
        uint32_t steps = 1u + ((bits >> 20) & 15u);
        struct motion result = integrate(position, velocity, acceleration, steps);
        uint32_t position_units = (uint32_t)(result.position * 512.0);
        uint32_t velocity_units = (uint32_t)(result.velocity * 512.0);
        hash = osp3_mix(hash, position_units);
        hash = osp3_mix(hash, velocity_units ^ steps);
    }
    printf("OSP3 scalar-double seed=%08x cases=%u digest=%u\n",
           (unsigned)OSP3_SEED, (unsigned)OSP3_CASE_BUDGET, (unsigned)hash);
    return 0;
}
