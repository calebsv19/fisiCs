extern int printf(const char*, ...);
extern int wave16_multitu_extent_fold(int values[2]);

int main(void) {
    int values[6] = {1, 4, 9, 16, 25, 36};
    printf("%d\n", wave16_multitu_extent_fold(values));
    return 0;
}
