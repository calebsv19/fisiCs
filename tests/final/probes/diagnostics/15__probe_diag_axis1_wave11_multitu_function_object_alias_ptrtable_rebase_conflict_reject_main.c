int axis1_wave11_diag_alias_a(void) {
    return 1;
}

int axis1_wave11_diag_alias_b = 2;

extern int axis1_wave11_diag_touch(void);

int main(void) {
    return axis1_wave11_diag_alias_a() + axis1_wave11_diag_alias_b + axis1_wave11_diag_touch();
}
