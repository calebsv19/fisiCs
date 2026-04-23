int axis1_wave13_diag_alias_a(void) {
    return 3;
}

int axis1_wave13_diag_alias_b = 4;

extern int axis1_wave13_diag_touch(void);

int main(void) {
    return axis1_wave13_diag_alias_a() + axis1_wave13_diag_alias_b + axis1_wave13_diag_touch();
}
