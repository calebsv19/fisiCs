#line 6320 "virtual_wave63_for_condition_incomplete_binary.c"
int wave63_condition_prefix(int value) {
    for (; value + ; ++value) {
        value += 100;
    }
    return value;
}

int wave63_condition_tail_global = 632;

int wave63_condition_tail_function(void) {
    return wave63_condition_tail_global + 1;
}
