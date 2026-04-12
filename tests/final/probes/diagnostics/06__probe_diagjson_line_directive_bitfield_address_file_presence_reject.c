#line 2101 "virtual_lv_bitfield_address_diagjson_probe.c"
struct ProbeBitfieldJson {
    unsigned b:3;
};

int probe_lv_bitfield_address_diagjson(void) {
    struct ProbeBitfieldJson s = {0};
    int *p = &s.b;
    return p != 0;
}
