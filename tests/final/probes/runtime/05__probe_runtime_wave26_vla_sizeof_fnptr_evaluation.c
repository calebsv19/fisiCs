extern int printf(const char*, ...);

struct Ops {
    int (*bound)(int);
    int offset;
};

static int calls;

static int make_bound(int value) {
    calls += 1;
    return value + 2;
}

int main(void) {
    struct Ops ops = {make_bound, 1};
    int trace = 4;
    int n = ops.bound(trace) + ops.offset;
    int vla_bytes = (int)sizeof(int[(trace += n, n)]);
    int ignored_bytes = (int)sizeof(ops.bound((trace += 100, 7)));

    printf("%d %d %d %d %d\n", calls, trace, n, vla_bytes, ignored_bytes);
    return 0;
}
