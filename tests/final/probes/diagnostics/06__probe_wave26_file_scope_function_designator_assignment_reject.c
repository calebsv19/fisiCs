static int wave26_target(int value) {
    return value + 4;
}

static int wave26_replacement(int value) {
    return value - 2;
}

int probe_wave26_assign_function_designator(void) {
#line 12601 "virtual_lv_wave26_function_designator_assignment.c"
    wave26_target = wave26_replacement;
    return wave26_target(9);
}
