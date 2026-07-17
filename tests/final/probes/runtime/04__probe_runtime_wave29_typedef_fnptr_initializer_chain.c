extern int printf(const char*, ...);

static int wave29_add(int a, int b) {
    return a + b;
}

static int wave29_mix(int a, int b) {
    return a * 3 - b;
}

static int wave29_typedef_fnptr_initializer_chain(void) {
    typedef int (*wave29_binary_t)(int, int);
    typedef wave29_binary_t wave29_binary_table_t[2];
    typedef wave29_binary_table_t* wave29_table_view_t;

    wave29_binary_table_t table = {wave29_add, wave29_mix};
    wave29_table_view_t table_view = &table;
    wave29_binary_t selected = (*table_view)[1], direct = table[0];
    int values[4] = {
        direct(7, 5),
        selected(6, 4),
        (int)(sizeof(table) / sizeof(table[0])),
        (*table_view)[0](3, 9),
    };
    return values[0] + values[1] + values[2] + values[3];
}

int main(void) {
    printf("%d\n", wave29_typedef_fnptr_initializer_chain());
    return 0;
}
