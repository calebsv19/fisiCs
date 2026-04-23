#ifndef AX4_W1_MIXED_ARITY_B_EXPECTED_H
#define AX4_W1_MIXED_ARITY_B_ACTUAL_H

#line 390201 "axis4_wave1_mixed_arity_layer_b.h"
#ifndef AX4_W1_MIX_REENTRY_COUNT
#define AX4_W1_MIX_REENTRY_COUNT 1
#define AX4_W1_MIX_NEEDN(a, b) ((a) + (b))
#else
#undef AX4_W1_MIX_NEEDN
#define AX4_W1_MIX_NEEDN(a, b, c) ((a) + (b) + (c))
#endif

#endif
