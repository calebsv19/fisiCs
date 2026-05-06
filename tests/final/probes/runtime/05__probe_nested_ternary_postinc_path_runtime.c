extern int printf(const char*, ...);

int main(void) {
    int a = 0;
    int b = 1;
    int c = 2;
    int value = a++ ? 100 : (b++ ? c++ : 77);
    printf("%d %d %d %d\n", a, b, c, value);
    return 0;
}
