extern int printf(const char*, ...);

static int wave24_add_scale_current(int base, int value) {
    return base + value * 2;
}

static int wave24_sub_scale_current(int base, int value) {
    return base - value;
}

static int wave24_fnptr_typedef_single_initializer_current(void) {
    typedef int (*op_t)(int, int);

    op_t add = wave24_add_scale_current;
    op_t sub = wave24_sub_scale_current;
    int init[3] = {
        add(10, 3),
        sub(20, 4),
        2,
    };

    return init[0] + init[1] + init[2];
}

int main(void) {
    printf("%d\n", wave24_fnptr_typedef_single_initializer_current());
    return 0;
}
