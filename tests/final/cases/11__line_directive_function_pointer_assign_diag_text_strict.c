#line 11901 "virtual_fn_function_pointer_assign_diag_text_strict.c"
int fi_text(int x) { return x; }
int fd_text(double x) { return (int)x; }

int main(void) {
    int (*fp)(int) = fi_text;
    int (*gp)(double) = fd_text;
    fp = gp;
    return fp(1);
}
