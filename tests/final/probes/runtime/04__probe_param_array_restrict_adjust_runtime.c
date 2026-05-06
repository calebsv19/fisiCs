extern int printf(const char*, ...);

int wave17_restrict_fold(int values[restrict 4]);

int wave17_restrict_fold(int *restrict values) {
    return values[0] * values[1] + values[2] - values[3];
}

int main(void) {
    int values[4] = {6, 3, 5, 2};
    printf("%d\n", wave17_restrict_fold(values));
    return 0;
}
