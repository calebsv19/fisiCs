#line 2351 "virtual_lv_include_compound_assign_pointer_plus_pointer_diagjson_probe.h"
static int probe_diagjson_lv_include_compound_assign_pointer_plus_pointer(void) {
    int a = 0;
    int b = 1;
    int *p = &a;
    int *q = &b;
    p += q;
    return 0;
}
