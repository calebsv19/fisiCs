extern int printf(const char*, ...);

int wave17_multitu_static_fold(int values[static 4]);

int main(void) {
    int values[4] = {8, 2, 7, 3};
    printf("%d\n", wave17_multitu_static_fold(values));
    return 0;
}
