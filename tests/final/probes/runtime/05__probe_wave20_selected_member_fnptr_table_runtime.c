extern int printf(const char*, ...);

struct Ops {
    int base;
    int (*apply)(int, int);
};

struct Slot {
    int weight;
    struct Ops ops;
};

static int add_pair(int left, int right) {
    return left + right;
}

static int scale_pair(int left, int right) {
    return left * right;
}

static int diff_pair(int left, int right) {
    return left - right;
}

int main(void) {
    struct Slot slots[3] = {
        {1, {2, add_pair}},
        {3, {4, scale_pair}},
        {5, {6, diff_pair}},
    };
    int state = 1;
    int pick = 0;

    struct Slot *selected =
        pick ? (state += 10, &slots[0])
             : (state += slots[2].weight, &slots[(state > 5) ? 1 : 0]);
    int arg = (state += selected->weight, state + selected->ops.base);
    int (*chosen)(int, int) = (arg > 10) ? selected->ops.apply : slots[0].ops.apply;
    int result = chosen(arg, selected->ops.base);

    printf("%d %d %d\n", state, arg, result);
    return 0;
}
