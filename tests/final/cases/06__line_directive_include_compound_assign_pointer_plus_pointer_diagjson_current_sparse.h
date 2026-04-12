#line 2151 "virtual_lv_include_compound_assign_pointer_plus_pointer_case.h"
static int lv_include_compound_assign_pointer_plus_pointer_case(void) {
    int a = 0;
    int b = 1;
    int *p = &a;
    int *q = &b;
    p += q;
    return 0;
}
