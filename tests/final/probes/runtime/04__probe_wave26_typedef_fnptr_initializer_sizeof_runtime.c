extern int printf(const char*, ...);

typedef int (*wave26_op_t)(int, int);
typedef wave26_op_t wave26_op_table_t[3];

static int wave26_add(int a, int b) {
    return a + b;
}

static int wave26_mul(int a, int b) {
    return a * b;
}

static int wave26_mix(int a, int b) {
    return a * 10 + b;
}

static int wave26_typedef_fnptr_initializer_sizeof(void) {
    wave26_op_table_t ops = {wave26_add, wave26_mul, wave26_mix};
    wave26_op_table_t* table_ref = &ops;
    int init[] = {
        (*table_ref)[0](7, 5),
        (*table_ref)[1](3, 4),
        (*table_ref)[2](2, 8),
        (int)(sizeof(ops) / sizeof(ops[0])),
    };
    return init[0] + init[1] + init[2] + init[3];
}

int main(void) {
    printf("%d\n", wave26_typedef_fnptr_initializer_sizeof());
    return 0;
}
