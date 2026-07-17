int wave64_clean_else_chain(int value) {
    if (value > 10) {
        value += 1;
    } else if (value > 0) {
        value += 2;
    } else {
        value += 3;
    }
    return value;
}

int wave64_clean_tail_global = 643;

int wave64_clean_tail_function(void) {
    return wave64_clean_tail_global + wave64_clean_else_chain(0);
}
