#line 2241 "virtual_lv_compound_assign_pointer_plus_pointer_diag_probe.c"
int main(void) {
    int a = 0;
    int b = 1;
    int *p = &a;
    int *q = &b;
    p += q;
    return 0;
}
