#line 2361 "virtual_lv_compound_assign_const_lvalue_diagjson_probe.c"
int main(void) {
    const int ci = 1;
    const int *pc = &ci;
    *pc += 1;
    return 0;
}
