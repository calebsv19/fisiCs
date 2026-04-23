#ifndef AX4_W7_B_C_H
#define AX4_W7_B_C_H

#line 531301 "axis4_wave7_perm_b_layer_c.h"
#ifndef AX4_W7B_PHASE
#define AX4_W7B_PHASE 1
#define AX4_W7B_CALL(a, b) ((a) + (b))
#else
#undef AX4_W7B_CALL
#define AX4_W7B_CALL(a, b, c) ((a) + (b) + (c))
#endif

#endif
