extern int printf(const char*, ...);

int wave16_const_fold(int values[const 4]);

int wave16_const_fold(int *values) {
    return values[0] * values[1] + values[2] - values[3];
}

int main(void) {
    int values[4] = {3, 5, 17, 4};
    printf("%d\n", wave16_const_fold(values));
    return 0;
}
