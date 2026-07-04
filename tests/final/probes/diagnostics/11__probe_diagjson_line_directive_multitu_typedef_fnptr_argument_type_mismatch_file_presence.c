#line 15801 "virtual_probe_fn_multitu_typedef_fnptr_argument_type_mismatch_diagjson_main.c"
typedef int (*ProbeW42PtrFn)(int *);

int probe_w42_need_ptr(int *p);

int main(void) {
    ProbeW42PtrFn fp = probe_w42_need_ptr;
    return fp(1);
}
