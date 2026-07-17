int wave62_prefix(int value) {
    int result = value;
    if (value > 0) {
        result++;
    }
    result += 2;
    return result;
}

int wave62_tail_global = 62;

int wave62_tail_function(void) {
    return wave62_tail_global + 1;
}
