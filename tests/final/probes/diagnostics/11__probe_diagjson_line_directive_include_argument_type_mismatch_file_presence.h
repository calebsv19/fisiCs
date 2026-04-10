#line 8801 "virtual_fn_include_argument_type_mismatch_probe.h"
int takes_ptr_inc(int *p) { return *p; }
int call_bad(void) { return takes_ptr_inc(1); }
