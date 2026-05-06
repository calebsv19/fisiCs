extern int printf(const char*, ...);

int wave17_static_fold(int values[static 4]);

int wave17_static_fold(int *values) {
    return values[0] * 2 + values[1] + values[2] - values[3];
}

int main(void) {
    int values[4] = {7, 4, 9, 5};
    printf("%d\n", wave17_static_fold(values));
    return 0;
}
