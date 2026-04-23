#line 545401 "axis4_wave10_inactive_layer_d.h"
#ifndef AX4_W10_INACTIVE_JOIN_PHASE
#define AX4_W10_INACTIVE_JOIN_PHASE 1
#if defined(AX4_W10_INACTIVE_BRANCH_LEFT)
#define AX4_W10_INACTIVE_JOIN_CALL(a, b) ((a) + (b))
#endif
#else
#if 0
#undef AX4_W10_INACTIVE_JOIN_CALL
#define AX4_W10_INACTIVE_JOIN_CALL(a, b, c) ((a) + (b) + (c))
#endif
#endif
