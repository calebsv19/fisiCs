#line 7401 "virtual_fn_include_too_many_args_case.h"
int add1_inc(int x) { return x; }
int call_bad(void) { return add1_inc(1, 2); }
