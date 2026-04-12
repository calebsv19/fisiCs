#line 13401 "virtual_probe_fn_multitu_include_too_many_args_diagjson.h"
int probe_mt_add1_inc(int x);
static int probe_mt_call_bad_inc(void) { return probe_mt_add1_inc(1, 2); }
