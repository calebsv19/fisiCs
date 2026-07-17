extern int printf(const char*, ...);

static int wave33_sum_fixed(const int values[static 4]);

static int wave33_sum_fixed(const int *values) {
    return values[0] + values[1] * 2 + values[2] * 3 + values[3] * 4;
}

int main(void) {
    typedef int (*wave33_sum_t)(const int [static 4]);
    const int values[4] = {2, 5, 7, 11};
    wave33_sum_t sum = wave33_sum_fixed;
    printf("%d\n", sum(values));
    return 0;
}
