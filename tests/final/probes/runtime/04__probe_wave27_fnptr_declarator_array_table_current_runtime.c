extern int printf(const char*, ...);

typedef int (*wave27_current_binary_op_t)(int, int);
typedef wave27_current_binary_op_t (*wave27_current_op_bank_t)[2][2];

static int wave27_current_add(int a, int b) {
    return a + b;
}

static int wave27_current_mul(int a, int b) {
    return a * b;
}

static int wave27_current_submix(int a, int b) {
    return a * 10 - b;
}

static int wave27_current_pack(int a, int b) {
    return a * 100 + b;
}

static int wave27_fnptr_declarator_array_table_current(void) {
    wave27_current_binary_op_t pairs[2][2] = {
        {wave27_current_add, wave27_current_mul},
        {wave27_current_submix, wave27_current_pack},
    };
    wave27_current_op_bank_t bank = &pairs;
    int calls[4] = {
        (*bank)[0][0](6, 4),
        (*bank)[0][1](3, 5),
        (*bank)[1][0](7, 2),
        (*bank)[1][1](1, 9),
    };
    return calls[0] + calls[1] + calls[2] + calls[3]
        + (int)(sizeof((*bank)[0]) / sizeof((*bank)[0][0]));
}

int main(void) {
    printf("%d\n", wave27_fnptr_declarator_array_table_current());
    return 0;
}
