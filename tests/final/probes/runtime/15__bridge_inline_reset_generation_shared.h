#ifndef FISICS_15_BRIDGE_INLINE_RESET_GENERATION_SHARED_H
#define FISICS_15_BRIDGE_INLINE_RESET_GENERATION_SHARED_H

static inline unsigned bridge15_scramble(unsigned x) {
    unsigned v = x + 0x9e3779b9u;
    v ^= v >> 16u;
    v *= 0x7feb352du;
    v ^= v >> 15u;
    v *= 0x846ca68bu;
    v ^= v >> 16u;
    return v;
}

static inline void bridge15_reset(unsigned* generation, unsigned seed) {
    *generation = bridge15_scramble(seed ^ 0xa5a5a5a5u);
}

#endif
