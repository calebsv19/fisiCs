#ifndef AX4_W8_ACTIVE_B_EXPECTED_H
#define AX4_W8_ACTIVE_B_ACTUAL_H

#line 541201 "axis4_wave8_active_layer_b.h"
#ifndef AX4_W8_ACTIVE_PHASE
#define AX4_W8_ACTIVE_PHASE 1
#define AX4_W8_ACTIVE_CALL(a, b) ((a) + (b))
#else
#undef AX4_W8_ACTIVE_CALL
#define AX4_W8_ACTIVE_CALL(a, b, c) ((a) + (b) + (c))
#endif

#endif
