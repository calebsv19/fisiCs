#line 1901 "virtual_lv_bitfield_address_diagjson_case.c"
struct Wave8BitsDiagjson {
    unsigned b:3;
};

int wave8_bitfield_address_diagjson_case(void) {
    struct Wave8BitsDiagjson s = {0};
    int *p = &s.b;
    return p != 0;
}
