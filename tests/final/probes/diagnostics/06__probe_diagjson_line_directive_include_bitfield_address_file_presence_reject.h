#line 2111 "virtual_lv_include_bitfield_address_diagjson_probe.h"
struct ProbeIncludeBitfieldJson {
    unsigned b:3;
};

static int probe_lv_include_bitfield_address_diagjson(void) {
    struct ProbeIncludeBitfieldJson s = {0};
    int *p = &s.b;
    return p != 0;
}
