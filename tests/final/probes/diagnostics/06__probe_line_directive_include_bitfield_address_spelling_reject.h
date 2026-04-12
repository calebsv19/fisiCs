#line 2071 "virtual_lv_include_bitfield_address_diag_probe.h"
struct ProbeIncludeBitfieldText {
    unsigned b:3;
};

static int probe_lv_include_bitfield_address_diag(void) {
    struct ProbeIncludeBitfieldText s = {0};
    int *p = &s.b;
    return p != 0;
}
