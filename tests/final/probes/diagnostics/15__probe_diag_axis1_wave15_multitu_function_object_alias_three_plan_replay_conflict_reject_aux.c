int axis1_wave15_diag_alias_b(void) {
    return 8;
}

int axis1_wave15_diag_alias_c = 10;

int axis1_wave15_diag_touch(void) {
    return axis1_wave15_diag_alias_b() + axis1_wave15_diag_alias_c;
}
