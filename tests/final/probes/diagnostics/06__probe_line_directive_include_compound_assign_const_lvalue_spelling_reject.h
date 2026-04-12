#line 2271 "virtual_lv_include_compound_assign_const_lvalue_diag_probe.h"
static int probe_lv_include_compound_assign_const_lvalue_diag(void) {
    const int ci = 1;
    const int *pc = &ci;
    *pc += 1;
    return 0;
}
