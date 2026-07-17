extern int printf(const char*, ...);

static int mark(int *slot, int amount) {
    *slot += amount;
    return *slot;
}

int main(void) {
    int a = 0;
    int b = 0;
    int c = 0;
    int gate = 1;

    int first = gate ? (mark(&a, 2), mark(&b, 3)) : (mark(&c, 50), 0);
    int second = (0 && mark(&c, 7)) ? mark(&a, 100) : (mark(&b, 5), first);
    int third = (1 || mark(&c, 11)) ? (mark(&a, 4), second + a) : mark(&b, 80);
    int fourth = (gate = 0, gate) ? mark(&c, 13) : (mark(&a, 1), mark(&b, 1));

    printf("%d %d %d %d %d\n", a, b, c, third, fourth);
    return 0;
}
