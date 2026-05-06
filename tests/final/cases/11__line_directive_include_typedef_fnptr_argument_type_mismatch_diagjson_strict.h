#line 15301 "virtual_fn_include_typedef_fnptr_argument_type_mismatch_diagjson_strict.h"
typedef int (*ProbePtrIncFn)(int *);

int probe_need_ptr_inc(int *p) { return *p; }

int run_probe_need_ptr_inc(void) {
    ProbePtrIncFn fp = probe_need_ptr_inc;
    return fp(1);
}
