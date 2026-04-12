#line 2001 "virtual_lv_include_bitfield_address_diag_case.h"
struct Wave8BitsIncludeDiag {
    unsigned b:3;
};

static int wave8_include_bitfield_address_diag_case(void) {
    struct Wave8BitsIncludeDiag s = {0};
    int *p = &s.b;
    return p != 0;
}
