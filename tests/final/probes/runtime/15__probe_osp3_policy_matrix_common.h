#ifndef FISICS_OSP3_POLICY_MATRIX_COMMON_H
#define FISICS_OSP3_POLICY_MATRIX_COMMON_H

#include <stdint.h>

#ifndef OSP3_CASE_BUDGET
#define OSP3_CASE_BUDGET 256u
#endif

#ifndef OSP3_DEFAULT_SEED
#define OSP3_DEFAULT_SEED 0x6d2b79f5u
#endif

#ifndef OSP3_SEED
#define OSP3_SEED OSP3_DEFAULT_SEED
#endif

static uint32_t osp3_next(uint32_t *state) {
    uint32_t value = *state;
    value ^= value << 13;
    value ^= value >> 17;
    value ^= value << 5;
    *state = value;
    return value;
}

static uint32_t osp3_mix(uint32_t hash, uint32_t value) {
    return hash ^ (value + 0x9e3779b9u + (hash << 6) + (hash >> 2));
}

#endif
