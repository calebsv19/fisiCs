extern int printf(const char*, ...);

int wave17_static_const_fold(int values[const static 5]);

int wave17_static_const_fold(int *values) {
    return values[0] + values[1] * values[2] + values[3] + values[4];
}

int main(void) {
    int values[5] = {3, 1, 4, 1, 5};
    printf("%d\n", wave17_static_const_fold(values));
    return 0;
}
