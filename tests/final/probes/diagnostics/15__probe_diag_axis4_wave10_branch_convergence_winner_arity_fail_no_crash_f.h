#line 544601 "axis4_wave10_winner_layer_f.h"
#ifndef AX4_W10_JOIN_PHASE
#define AX4_W10_JOIN_PHASE 1
#if defined(AX4_W10_BRANCH_LEFT) && !defined(AX4_W10_BRANCH_RIGHT)
#define AX4_W10_JOIN_CALL(a, b) ((a) + (b))
#endif
#else
#if defined(AX4_W10_BRANCH_RIGHT)
#undef AX4_W10_JOIN_CALL
#define AX4_W10_JOIN_CALL(a, b, c) ((a) + (b) + (c))
#endif
#endif
