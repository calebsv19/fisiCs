#ifndef AXIS2_WAVE4_TOKEN_BRIDGE_D_H
#define AXIS2_WAVE4_TOKEN_BRIDGE_D_H

#line 170401 "axis2_wave4_token_layer_d.h"
#define AX2_W4_TOKEN_JOIN(prefix, name) prefix##name
#define AX2_W4_TOKEN_BUILD(name) AX2_W4_TOKEN_JOIN(axis2_wave4_token_missing_, name)
#define AX2_W4_TOKEN_STAGE3(v) ((v) + AX2_W4_TOKEN_BUILD(symbol))

static int axis2_wave4_token_eval(int seed) {
    return AX2_W4_TOKEN_STAGE1(seed);
}

#endif
