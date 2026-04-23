#ifndef AXIS2_WAVE3_NESTED_ARITY_D_H
#define AXIS2_WAVE3_NESTED_ARITY_D_H

#line 130401 "axis2_wave3_nested_layer_d.h"
static int axis2_wave3_nested_binary(int left, int right) {
    return left + right;
}

#define AX2_W3_NESTED_CALL_BASE(v) axis2_wave3_nested_binary((v))

static int axis2_wave3_nested_eval(int seed) {
    return AX2_W3_NESTED_CALL_STAGE1(seed);
}

#endif
