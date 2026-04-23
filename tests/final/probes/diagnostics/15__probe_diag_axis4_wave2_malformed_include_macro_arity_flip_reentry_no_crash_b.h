#ifndef AX4_W2_ARITY_FLIP_B_EXPECTED_H
#define AX4_W2_ARITY_FLIP_B_ACTUAL_H

#line 481201 "axis4_wave2_arity_flip_layer_b.h"
#ifndef AX4_W2_ARITY_FLIP_PHASE
#define AX4_W2_ARITY_FLIP_PHASE 1
#define AX4_W2_NEED(a, b) ((a) + (b))
#else
#undef AX4_W2_NEED
#define AX4_W2_NEED(a, b, c) ((a) + (b) + (c))
#endif

#endif
