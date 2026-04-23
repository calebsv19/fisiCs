int axis1_wave15_diag_alias_a(void) {
    return 5;
}

int axis1_wave15_diag_alias_b = 6;

extern int axis1_wave15_diag_touch(void);

int main(void) {
    return axis1_wave15_diag_alias_a() + axis1_wave15_diag_alias_b + axis1_wave15_diag_touch();
}
