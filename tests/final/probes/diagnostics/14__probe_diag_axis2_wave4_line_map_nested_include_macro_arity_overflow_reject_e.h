#ifndef AXIS2_WAVE4_ARITY_OVERFLOW_E_H
#define AXIS2_WAVE4_ARITY_OVERFLOW_E_H

#line 160501 "axis2_wave4_arity_layer_e.h"
static int axis2_wave4_arity_leaf(int left) {
    return left * 2;
}

#define AX2_W4_ARITY_BASE(v) axis2_wave4_arity_leaf((v), (v) + 9)

static int axis2_wave4_arity_eval(int seed) {
    return AX2_W4_ARITY_STAGE1(seed);
}

#endif
