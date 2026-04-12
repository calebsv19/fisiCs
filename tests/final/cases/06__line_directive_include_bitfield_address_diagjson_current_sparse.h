#line 1921 "virtual_lv_include_bitfield_address_diagjson_case.h"
struct Wave8BitsIncludeDiagjson {
    unsigned b:3;
};

static int wave8_include_bitfield_address_diagjson_case(void) {
    struct Wave8BitsIncludeDiagjson s = {0};
    int *p = &s.b;
    return p != 0;
}
