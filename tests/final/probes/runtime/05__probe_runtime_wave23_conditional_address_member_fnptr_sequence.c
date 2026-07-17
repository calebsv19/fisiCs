extern int printf(const char*, ...);

struct Slot {
    int base;
    int (*mix)(int*, int);
};

static int mix_a(int *trace, int value) {
    *trace += value + 1;
    return *trace;
}

static int mix_b(int *trace, int value) {
    *trace += value + 3;
    return *trace + 10;
}

int main(void) {
    struct Slot left = {2, mix_a};
    struct Slot right = {5, mix_b};
    int trace = 1;
    int choose = 1;

    struct Slot *picked = (trace += 4, choose ? &left : &right);
    int first = (picked->mix)(&trace, (trace += 2, picked->base));
    choose = 0;
    trace += 7;
    int second_arg = (trace += 11, right.base);
    int second = (choose ? &left : &right)->mix(&trace, second_arg);

    printf("%d %d %d %d\n", trace, first, second, right.base);
    return 0;
}
