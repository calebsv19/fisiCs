#line 16101 "virtual_fn_multitu_include_typedef_fnptr_argument_type_mismatch_diagjson_wave43_header.h"
typedef int (*W43ProbePtrIncFn)(int *);

int w43_probe_need_ptr_inc(int *p);

static int w43_run_probe_need_ptr_inc(void) {
    W43ProbePtrIncFn fp = w43_probe_need_ptr_inc;
    return fp(1);
}
