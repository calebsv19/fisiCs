#ifndef AXIS2_WAVE3_GUARD_TYPE_D_H
#define AXIS2_WAVE3_GUARD_TYPE_D_H

#line 120401 "axis2_wave3_guard_layer_d.h"
#define AX2_W3_GUARD_BASE_EXPR(v) ((v) + axis2_wave3_guard_missing_symbol)

static int axis2_wave3_guard_eval(int seed) {
    return AX2_W3_GUARD_WRAP_STAGE1(seed);
}

#endif
