#line 1981 "virtual_lv_bitfield_address_diag_case.c"
struct Wave8BitsDiag {
    unsigned b:3;
};

int wave8_bitfield_address_diag_case(void) {
    struct Wave8BitsDiag s = {0};
    int *p = &s.b;
    return p != 0;
}
