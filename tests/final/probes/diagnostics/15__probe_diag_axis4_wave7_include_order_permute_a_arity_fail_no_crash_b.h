#ifndef AX4_W7_A_B_H
#define AX4_W7_A_B_H

#line 530201 "axis4_wave7_perm_a_layer_b.h"
#ifndef AX4_W7A_PHASE
#define AX4_W7A_PHASE 1
#define AX4_W7A_CALL(a, b) ((a) + (b))
#else
#undef AX4_W7A_CALL
#define AX4_W7A_CALL(a, b, c) ((a) + (b) + (c))
#endif

#endif
