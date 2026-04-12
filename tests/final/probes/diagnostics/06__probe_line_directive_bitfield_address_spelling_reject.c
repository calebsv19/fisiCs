#line 2061 "virtual_lv_bitfield_address_diag_probe.c"
struct ProbeBitfieldText {
    unsigned b:3;
};

int probe_lv_bitfield_address_diag(void) {
    struct ProbeBitfieldText s = {0};
    int *p = &s.b;
    return p != 0;
}
