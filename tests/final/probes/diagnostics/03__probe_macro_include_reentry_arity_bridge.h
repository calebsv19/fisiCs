#line 420 "virtual_macro_reentry_header.h"
#define REENTRY_TRIAD(a, b, c) ((a) + (b) + (c))

#ifndef REENTRY_MACRO_BRIDGE_SEEN
#define REENTRY_MACRO_BRIDGE_SEEN 1
#define REENTRY_BRIDGE(x) REENTRY_TRIAD(x, 2)
#else
#undef REENTRY_BRIDGE
#define REENTRY_BRIDGE(x) REENTRY_TRIAD(x, 2)
#endif

int macro_include_reentry_arity_bridge = REENTRY_BRIDGE(1);
