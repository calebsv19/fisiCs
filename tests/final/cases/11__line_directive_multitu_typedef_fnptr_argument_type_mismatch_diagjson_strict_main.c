#line 15601 "virtual_fn_multitu_typedef_fnptr_argument_type_mismatch_diagjson_wave42_main.c"
typedef int (*W42ProbePtrFn)(int *);

int w42_probe_need_ptr(int *p);

int main(void) {
    W42ProbePtrFn fp = w42_probe_need_ptr;
    return fp(1);
}
