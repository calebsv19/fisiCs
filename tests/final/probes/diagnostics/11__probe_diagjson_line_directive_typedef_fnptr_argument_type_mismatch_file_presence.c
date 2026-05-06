#line 15401 "virtual_probe_fn_typedef_fnptr_argument_type_mismatch_diagjson.c"
typedef int (*ProbePtrFn)(int *);

int probe_need_ptr(int *p) { return *p; }

int main(void) {
    ProbePtrFn fp = probe_need_ptr;
    return fp(1);
}
