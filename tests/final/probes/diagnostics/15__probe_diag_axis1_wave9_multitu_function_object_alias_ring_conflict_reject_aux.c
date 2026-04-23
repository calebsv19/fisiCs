int axis1_wave9_diag_alias_b(void) {
    return 3;
}

int axis1_wave9_diag_alias_c = 5;

int axis1_wave9_diag_touch(void) {
    return axis1_wave9_diag_alias_b() + axis1_wave9_diag_alias_c;
}
