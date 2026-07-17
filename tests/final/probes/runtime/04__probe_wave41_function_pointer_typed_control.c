static int wave41_apply_typed(int (*callback)(int));

static int wave41_apply_typed(int (*callback)(int)) {
    return callback(41);
}

static int wave41_increment_typed(int value) {
    return value + 1;
}

int main(void) {
    return wave41_apply_typed(wave41_increment_typed) == 42 ? 0 : 1;
}
