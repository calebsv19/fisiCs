#line 2371 "virtual_lv_include_compound_assign_const_lvalue_diagjson_probe.h"
static int probe_diagjson_lv_include_compound_assign_const_lvalue(void) {
    const int ci = 1;
    const int *pc = &ci;
    *pc += 1;
    return 0;
}
