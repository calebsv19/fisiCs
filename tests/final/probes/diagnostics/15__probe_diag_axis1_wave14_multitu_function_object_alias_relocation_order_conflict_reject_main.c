int axis1_wave14_diag_alias_a(void) {
    return 4;
}

int axis1_wave14_diag_alias_b = 5;

extern int axis1_wave14_diag_touch(void);

int main(void) {
    return axis1_wave14_diag_alias_a() + axis1_wave14_diag_alias_b + axis1_wave14_diag_touch();
}
