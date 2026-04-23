#ifndef AX4_W5_ACTIVE_B_EXPECTED_H
#define AX4_W5_ACTIVE_B_ACTUAL_H

#line 511201 "axis4_wave5_active_branch_layer_b.h"
#ifndef AX4_W5_ACTIVE_PHASE
#define AX4_W5_ACTIVE_PHASE 1
#define AX4_W5_ACTIVE_CALL(a, b) ((a) + (b))
#else
#undef AX4_W5_ACTIVE_CALL
#define AX4_W5_ACTIVE_CALL(a, b, c) ((a) + (b) + (c))
#endif

#endif
