#line 2171 "virtual_lv_include_compound_assign_const_lvalue_case.h"
static int lv_include_compound_assign_const_lvalue_case(void) {
    const int ci = 1;
    const int *pc = &ci;
    *pc += 1;
    return 0;
}
