#line 2251 "virtual_lv_include_compound_assign_pointer_plus_pointer_diag_probe.h"
static int probe_lv_include_compound_assign_pointer_plus_pointer_diag(void) {
    int a = 0;
    int b = 1;
    int *p = &a;
    int *q = &b;
    p += q;
    return 0;
}
