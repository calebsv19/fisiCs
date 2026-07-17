extern int printf(const char*, ...);

static int wave24_add_scale(int base, int value) {
    return base + value * 2;
}

static int wave24_sub_scale(int base, int value) {
    return base - value;
}

static int wave24_fnptr_typedef_table_sizeof_initializer(void) {
    typedef int (*op_t)(int, int);
    typedef op_t op_table_t[2];
    typedef op_table_t* op_table_ptr_t;

    op_table_t table = {wave24_add_scale, wave24_sub_scale};
    op_table_ptr_t table_ref = &table;
    int init[3] = {
        (*table_ref)[0](10, 3),
        (*table_ref)[1](20, 4),
        (int)(sizeof(*table_ref) / sizeof((*table_ref)[0])),
    };

    return init[0] + init[1] + init[2];
}

int main(void) {
    printf("%d\n", wave24_fnptr_typedef_table_sizeof_initializer());
    return 0;
}
