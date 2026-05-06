extern int printf(const char*, ...);

int wave16_pointer_fold(int values[5]);

int wave16_pointer_fold(int *values) {
    return values[1] - values[0] + values[4];
}

int main(void) {
    int values[5] = {4, 9, 2, 7, 15};
    printf("%d\n", wave16_pointer_fold(values));
    return 0;
}
