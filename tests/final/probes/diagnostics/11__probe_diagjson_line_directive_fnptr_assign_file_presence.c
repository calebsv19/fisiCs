#line 12311 "virtual_fn_diagjson_fnptr_assign_probe.c"
int fi_probe(int x) { return x; }
int fd_probe(double x) { return (int)x; }

int main(void) {
    int (*fp)(int) = fi_probe;
    int (*gp)(double) = fd_probe;
    fp = gp;
    return fp(1);
}
