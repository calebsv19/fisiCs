extern int printf(const char*, ...);

static int wave30_fixed_sum(int (*row)[4]) {
    return (*row)[0] + (*row)[3];
}

static int wave30_fixed_mix(int (*row)[4]) {
    return (*row)[1] * 2 - (*row)[2];
}

static int wave30_fixed_fnptr_initializer_current(void) {
    typedef int wave30_fixed_row_t[4];
    typedef wave30_fixed_row_t* wave30_fixed_view_t;
    typedef int (*wave30_fixed_pick_t)(wave30_fixed_view_t);
    typedef wave30_fixed_pick_t wave30_fixed_table_t[2];

    wave30_fixed_row_t rows[2] = {
        {3, 5, 7, 11},
        {13, 17, 19, 23},
    };
    wave30_fixed_table_t table = {wave30_fixed_sum, wave30_fixed_mix};
    wave30_fixed_view_t first = &rows[0], second = &rows[1];
    int values[4] = {
        table[0](first),
        table[1](second),
        (int)sizeof(first[0]),
        second[0][2],
    };
    return values[0] + values[1] + values[2] + values[3];
}

int main(void) {
    printf("%d\n", wave30_fixed_fnptr_initializer_current());
    return 0;
}
