extern int printf(const char*, ...);

int main(void) {
    int n = 3;
    int value = 1 ? (int)(sizeof(int[(n += 2, n)]) / sizeof(int)) : 77;
    printf("%d %d\n", n, value);
    return 0;
}
