extern int printf(const char*, ...);

typedef int wave22_values5_t[5];

int wave22_typedef_array_fold(wave22_values5_t values);

int wave22_typedef_array_fold(wave22_values5_t values) {
    return values[0] + values[2] + values[4];
}

int main(void) {
    wave22_values5_t values = {2, 4, 6, 8, 10};
    printf("%d\n", wave22_typedef_array_fold(values));
    return 0;
}
