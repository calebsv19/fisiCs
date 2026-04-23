int axis1_wave7_alias_a(void) {
    return 7;
}

int axis1_wave7_alias_b = 3;

extern int axis1_wave7_touch(void);

int main(void) {
    return axis1_wave7_alias_a() + axis1_wave7_alias_b + axis1_wave7_touch();
}
