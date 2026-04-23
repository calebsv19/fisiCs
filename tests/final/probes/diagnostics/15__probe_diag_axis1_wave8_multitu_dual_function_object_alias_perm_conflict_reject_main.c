int axis1_wave8_diag_alias_f(void) {
    return 1;
}

int axis1_wave8_diag_alias_g = 2;

extern int axis1_wave8_diag_touch(void);

int main(void) {
    return axis1_wave8_diag_alias_f() + axis1_wave8_diag_alias_g + axis1_wave8_diag_touch();
}
