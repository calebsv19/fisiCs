#ifndef AX4_W7_A_C_H
#define AX4_W7_A_C_H

#line 530301 "axis4_wave7_perm_a_layer_c.h"
#ifndef AX4_W7A_PHASE
#define AX4_W7A_PHASE 1
#define AX4_W7A_CALL(a, b) ((a) + (b))
#else
#undef AX4_W7A_CALL
#define AX4_W7A_CALL(a, b, c) ((a) + (b) + (c))
#endif

#endif
