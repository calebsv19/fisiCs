int axis1_wave12_diag_alias_a(void) {
    return 2;
}

int axis1_wave12_diag_alias_b = 3;

extern int axis1_wave12_diag_touch(void);

int main(void) {
    return axis1_wave12_diag_alias_a() + axis1_wave12_diag_alias_b + axis1_wave12_diag_touch();
}
