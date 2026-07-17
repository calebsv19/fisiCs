extern int printf(const char*, ...);

typedef int (*wave28_op_t)(int, int);
typedef wave28_op_t (*wave28_selector_t)(int);

static int wave28_add3(int a, int b) {
    return a + b + 3;
}

static int wave28_mix5(int a, int b) {
    return a * 5 - b;
}

static int wave28_pack7(int a, int b) {
    return a * 100 + b * 10 + 7;
}

static wave28_op_t wave28_select_a(int tag) {
    return tag ? wave28_mix5 : wave28_add3;
}

static wave28_op_t wave28_select_b(int tag) {
    return tag ? wave28_pack7 : wave28_mix5;
}

static int wave28_fnptr_selector_initializer(void) {
    typedef wave28_selector_t wave28_selector_row_t[2];
    typedef wave28_selector_row_t* wave28_selector_bank_t;

    wave28_selector_row_t selectors = {wave28_select_a, wave28_select_b};
    wave28_selector_bank_t bank = &selectors;
    wave28_op_t ops[3] = {
        (*bank)[0](0),
        (*bank)[0](1),
        (*bank)[1](1),
    };
    int calls[4] = {
        ops[0](4, 5),
        ops[1](6, 2),
        ops[2](3, 8),
        (int)(sizeof(*bank) / sizeof((*bank)[0])),
    };
    return calls[0] + calls[1] + calls[2] + calls[3];
}

int main(void) {
    printf("%d\n", wave28_fnptr_selector_initializer());
    return 0;
}
