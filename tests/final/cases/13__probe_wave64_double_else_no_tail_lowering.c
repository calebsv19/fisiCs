#line 6420 "virtual_wave64_double_else.c"
int wave64_double_else_prefix(int value) {
    if (value) {
        value += 1;
    } else else {
        value += 2;
    }
    return value;
}

int wave64_double_else_tail_global = 642;

int wave64_double_else_tail_function(void) {
    return wave64_double_else_tail_global + 1;
}
