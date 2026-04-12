#line 14601 "virtual_probe_fn_multitu_include_fnptr_assign_diagjson_header.h"
int probe_wf20_ifi(int x);
int probe_wf20_ifd(double x);
static int probe_wf20_bad_call(void) {
    int (*fp)(int) = probe_wf20_ifi;
    int (*gp)(double) = probe_wf20_ifd;
    fp = gp;
    return fp(1);
}
