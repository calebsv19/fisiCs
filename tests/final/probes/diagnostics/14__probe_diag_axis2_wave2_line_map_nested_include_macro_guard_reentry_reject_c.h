#ifndef AXIS2_WAVE2_GUARD_REENTRY_C_H
#define AXIS2_WAVE2_GUARD_REENTRY_C_H

#line 240301 "axis2_wave2_guard_layer_c.h"
struct Axis2Wave2GuardValue {
    int lane;
};

#define AX2_W2_GUARD_NONSCALAR(seed) ((struct Axis2Wave2GuardValue){(seed)})

static int axis2_wave2_guard_eval(int seed) {
    if (AX2_W2_GUARD_NONSCALAR(seed)) {
        return seed;
    }
    return 0;
}

#endif
