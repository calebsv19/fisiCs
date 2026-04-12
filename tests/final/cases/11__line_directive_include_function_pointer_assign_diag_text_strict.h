#line 12001 "virtual_fn_include_function_pointer_assign_diag_text_strict.h"
int fi_inc_text(int x) { return x; }
int fd_inc_text(double x) { return (int)x; }
int run_fnptr_bad_text(void) {
    int (*fp)(int) = fi_inc_text;
    int (*gp)(double) = fd_inc_text;
    fp = gp;
    return fp(1);
}
