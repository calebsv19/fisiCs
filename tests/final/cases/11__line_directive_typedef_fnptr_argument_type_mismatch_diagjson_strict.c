#line 15201 "virtual_fn_typedef_fnptr_argument_type_mismatch_diagjson_strict.c"
typedef int (*ProbePtrFn)(int *);

int probe_need_ptr(int *p) { return *p; }

int main(void) {
    ProbePtrFn fp = probe_need_ptr;
    return fp(1);
}
