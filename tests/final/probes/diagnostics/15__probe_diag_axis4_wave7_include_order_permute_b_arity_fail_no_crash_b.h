#ifndef AX4_W7_B_B_H
#define AX4_W7_B_B_H

#line 531201 "axis4_wave7_perm_b_layer_b.h"
#ifndef AX4_W7B_PHASE
#define AX4_W7B_PHASE 1
#define AX4_W7B_CALL(a, b) ((a) + (b))
#else
#undef AX4_W7B_CALL
#define AX4_W7B_CALL(a, b, c) ((a) + (b) + (c))
#endif

#endif
