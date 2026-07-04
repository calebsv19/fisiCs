#line 16101 "virtual_probe_fn_multitu_include_typedef_fnptr_argument_type_mismatch_diagjson_wave43_header.h"
typedef int (*ProbeW43PtrIncFn)(int *);

int probe_w43_need_ptr_inc(int *p);

static int run_probe_w43_need_ptr_inc(void) {
    ProbeW43PtrIncFn fp = probe_w43_need_ptr_inc;
    return fp(1);
}
