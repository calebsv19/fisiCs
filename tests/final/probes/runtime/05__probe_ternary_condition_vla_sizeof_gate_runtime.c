extern int printf(const char*, ...);

int main(void) {
    int n = 2;
    int gate = 0;
    int value = (n += 1, gate = (int)(sizeof(int[(n += 2, n)]) / sizeof(int)), 0) ? 11 : n;
    printf("%d %d\n", n, value);
    return 0;
}
