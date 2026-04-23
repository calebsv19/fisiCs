int axis1_wave14_diag_alias_b(void) {
    return 7;
}

int axis1_wave14_diag_alias_c = 9;

int axis1_wave14_diag_touch(void) {
    return axis1_wave14_diag_alias_b() + axis1_wave14_diag_alias_c;
}
