#line 16001 "virtual_fn_multitu_typedef_fnptr_argument_type_mismatch_diagjson_wave43_main.c"
typedef int (*W43ProbePtrFn)(int *);

int w43_probe_need_ptr(int *p);

int main(void) {
    W43ProbePtrFn fp = w43_probe_need_ptr;
    return fp(1);
}
