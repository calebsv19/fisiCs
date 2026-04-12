#line 12411 "virtual_fn_diagjson_include_fnptr_assign_probe.h"
int fi_inc_probe(int x) { return x; }
int fd_inc_probe(double x) { return (int)x; }
int run_probe_diagjson_fnptr_include(void) {
    int (*fp)(int) = fi_inc_probe;
    int (*gp)(double) = fd_inc_probe;
    fp = gp;
    return fp(1);
}
