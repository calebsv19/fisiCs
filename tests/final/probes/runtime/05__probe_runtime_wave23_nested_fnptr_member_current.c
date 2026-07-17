extern int printf(const char*, ...);

struct Ops {
    int delta;
    int (*step)(int, int);
};

static int plus(int value, int delta) {
    return value + delta;
}

static int times(int value, int delta) {
    return value * delta;
}

int main(void) {
    struct Ops ops[3] = {{2, plus}, {3, times}, {5, plus}};
    int trace = 4;
    int choose_outer = 1;
    int choose_inner = 0;

    struct Ops *picked = choose_outer ? (choose_inner ? &ops[0] : &ops[1]) : &ops[2];
    int value = picked->step((trace += 6), picked->delta);
    choose_inner = 1;
    struct Ops *picked2 = choose_outer ? (choose_inner ? &ops[0] : &ops[1]) : &ops[2];
    int value2 = picked2->step((trace += 7), picked2->delta);

    printf("%d %d %d\n", trace, value, value2);
    return 0;
}
