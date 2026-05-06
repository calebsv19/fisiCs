extern int printf(const char*, ...);

int wave16_extent_fold(int values[3]);

int wave16_extent_fold(int values[7]) {
    return values[0] + values[3] + values[6];
}

int main(void) {
    int values[7] = {2, 5, 7, 11, 13, 17, 19};
    printf("%d\n", wave16_extent_fold(values));
    return 0;
}
