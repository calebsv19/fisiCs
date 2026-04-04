struct op_entry {
    const char* name;
    int (*fn)(int, int);
};

static int op_add(int a, int b) {
    return a + b;
}

static int op_sub(int a, int b) {
    return a - b;
}

const struct op_entry linkage_ops[] = {
    {"add", op_add},
    {"sub", op_sub}
};

int linkage_ops_count(void) {
    return 2;
}
