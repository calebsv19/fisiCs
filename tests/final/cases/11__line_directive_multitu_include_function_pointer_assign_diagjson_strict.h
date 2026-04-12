#line 14201 "virtual_fn_multitu_include_fnptr_assign_diagjson_wave20_header.h"
int wf20_ifi(int x);
int wf20_ifd(double x);
static int wf20_bad_call(void) {
    int (*fp)(int) = wf20_ifi;
    int (*gp)(double) = wf20_ifd;
    fp = gp;
    return fp(1);
}
