#line 15701 "virtual_fn_multitu_include_typedef_fnptr_argument_type_mismatch_diagjson_wave42_header.h"
typedef int (*W42ProbePtrIncFn)(int *);

int w42_probe_need_ptr_inc(int *p);

static int w42_run_probe_need_ptr_inc(void) {
    W42ProbePtrIncFn fp = w42_probe_need_ptr_inc;
    return fp(1);
}
