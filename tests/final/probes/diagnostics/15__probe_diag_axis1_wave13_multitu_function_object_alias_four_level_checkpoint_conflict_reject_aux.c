int axis1_wave13_diag_alias_b(void) {
    return 6;
}

int axis1_wave13_diag_alias_c = 8;

int axis1_wave13_diag_touch(void) {
    return axis1_wave13_diag_alias_b() + axis1_wave13_diag_alias_c;
}
