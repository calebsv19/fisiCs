#ifndef AXIS2_WAVE2_NESTED_CHAIN_C_H
#define AXIS2_WAVE2_NESTED_CHAIN_C_H

#line 340301 "axis2_wave2_nested_layer_c.h"
#define AX2_W2_NESTED_EXPR(seed) ((seed) + axis2_wave2_nested_missing_symbol)

static int axis2_wave2_nested_eval(int seed) {
    return AX2_W2_NESTED_EXPR(seed + 3);
}

#endif
