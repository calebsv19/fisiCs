int axis1_wave8_diag_alias_g(void) {
    return 3;
}

int axis1_wave8_diag_alias_h = 5;

int axis1_wave8_diag_touch(void) {
    return axis1_wave8_diag_alias_g() + axis1_wave8_diag_alias_h;
}
