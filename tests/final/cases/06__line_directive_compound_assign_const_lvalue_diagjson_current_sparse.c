#line 2161 "virtual_lv_compound_assign_const_lvalue_case.c"
int lv_compound_assign_const_lvalue_case(void) {
    const int ci = 1;
    const int *pc = &ci;
    *pc += 1;
    return 0;
}
