#ifndef AX4_W4_ARITY_B_EXPECTED_H
#define AX4_W4_ARITY_B_ACTUAL_H

#line 501201 "axis4_wave4_arity_layer_b.h"
#ifndef AX4_W4_ARITY_PHASE
#define AX4_W4_ARITY_PHASE 1
#define AX4_W4_NEEDF(a, b, c, d) ((a) + (b) + (c) + (d))
#else
#undef AX4_W4_NEEDF
#define AX4_W4_NEEDF(a, b, c, d, e) ((a) + (b) + (c) + (d) + (e))
#endif

#endif
