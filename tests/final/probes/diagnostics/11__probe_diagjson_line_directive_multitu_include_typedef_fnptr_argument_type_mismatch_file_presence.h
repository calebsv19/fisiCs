#line 15901 "virtual_probe_fn_multitu_include_typedef_fnptr_argument_type_mismatch_diagjson_header.h"
typedef int (*ProbeW42PtrIncFn)(int *);

int probe_w42_need_ptr_inc(int *p);

static int run_probe_w42_need_ptr_inc(void) {
    ProbeW42PtrIncFn fp = probe_w42_need_ptr_inc;
    return fp(1);
}
