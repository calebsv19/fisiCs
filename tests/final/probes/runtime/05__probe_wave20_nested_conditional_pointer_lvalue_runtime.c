extern int printf(const char*, ...);

int main(void) {
    int a = 4;
    int b = 9;
    int c = 2;
    int trace = 0;

    int *slot = (trace += 1, a < b) ? (trace += 2, &a) : (trace += 20, &b);
    *slot += c ? (trace += 3, c + 5) : (trace += 30, 0);
    int rvalue = (a > b) ? (trace += 4, a - b) : (trace += 40, b - a);
    int nested = rvalue ? (trace += 5, *slot += 1, *slot)
                        : (trace += 50, b += 1, b);

    printf("%d %d %d %d %d\n", a, b, c, trace, nested);
    return 0;
}
