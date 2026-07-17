#line 310 "virtual_macro_callsite_arity.c"
#define CALLSITE_PAIR(a, b) ((a) + (b))
#define CALLSITE_BRIDGE(x) CALLSITE_PAIR(x)
int macro_callsite_line_remap_arity = CALLSITE_BRIDGE(7);
