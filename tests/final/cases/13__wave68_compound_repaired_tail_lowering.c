#line 6850 "virtual_wave68_compound_repaired.c"
int wave68_repaired_prefix_function(void) {
    int wave68_target = 4;
    int wave68_values[4] = { 1, 2, 3, 4 };
    int *wave68_pointer = wave68_values;
    wave68_target += 1;
    wave68_pointer += 2;
    return *wave68_pointer;
}

int wave68_repaired_tail_global = 685;

int wave68_repaired_tail_function(void) {
    return wave68_repaired_tail_global + 1;
}
