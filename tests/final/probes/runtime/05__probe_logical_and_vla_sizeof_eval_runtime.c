extern int printf(const char*, ...);

int main(void) {
    int n = 2;
    int value = 1 && (int)(sizeof(int[(n += 3, n)]) / sizeof(int));
    printf("%d %d\n", n, value);
    return 0;
}
