int axis1_wave12_diag_alias_b(void) {
    return 4;
}

int axis1_wave12_diag_alias_c = 6;

int axis1_wave12_diag_touch(void) {
    return axis1_wave12_diag_alias_b() + axis1_wave12_diag_alias_c;
}
