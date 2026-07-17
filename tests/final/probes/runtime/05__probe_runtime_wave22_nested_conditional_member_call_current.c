extern int printf(const char*, ...);

struct Ops {
    int bias;
    int (*fn)(int, int);
};

static int add(int x, int y) {
    return x + y;
}

static int mix(int x, int y) {
    return x * 2 - y;
}

int main(void) {
    struct Ops ops[3] = {{1, add}, {3, mix}, {5, add}};
    int idx = 0;
    int trace = 4;

    struct Ops *chosen = ((trace += 1), trace & 1)
        ? &ops[(idx += 1)]
        : (((trace += 20), idx = 2), &ops[idx]);
    int first = chosen->fn(trace, chosen->bias);

    struct Ops *chosen2 = ((trace += first), idx == 1)
        ? &ops[(trace += 2, 2)]
        : &ops[0];
    int second = chosen2->fn(trace, (int)sizeof(chosen2->bias) + chosen->bias);

    printf("%d %d %d %d\n", idx, trace, first, second);
    return 0;
}
