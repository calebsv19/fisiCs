int wave63_clean_prefix(int value) {
    for (; 0; ) {
        value += 100;
    }
    return value;
}

int wave63_clean_tail_global = 633;

int wave63_clean_tail_function(void) {
    return wave63_clean_tail_global + 1;
}
