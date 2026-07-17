extern int printf(const char*, ...);

struct Box {
    int value;
    int (*fn)(int);
};

static int bump(int value) {
    return value + 11;
}

static int drop(int value) {
    return value - 2;
}

int main(void) {
    struct Box left = {4, bump};
    struct Box right = {9, drop};
    struct Box *picked = &left;
    int trace = 1;
    int guard = 1;

    int first = guard || ((picked = &right), right.fn(trace += 5));
    int second = (guard && ((trace += 3), picked->fn(picked->value))) ? (trace += picked->value) : (trace += 100);

    guard = 0;
    int third = guard && ((picked = &right), (trace += picked->value));

    printf("%d %d %d %d %d\n", trace, first, second, third, picked->value);
    return 0;
}
