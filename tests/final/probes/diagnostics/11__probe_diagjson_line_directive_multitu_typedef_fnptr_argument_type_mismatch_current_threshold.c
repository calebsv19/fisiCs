#line 16001 "virtual_probe_fn_multitu_typedef_fnptr_argument_type_mismatch_diagjson_wave43_main.c"
typedef int (*ProbeW43PtrFn)(int *);

int probe_w43_need_ptr(int *p);

int main(void) {
    ProbeW43PtrFn fp = probe_w43_need_ptr;
    return fp(1);
}
