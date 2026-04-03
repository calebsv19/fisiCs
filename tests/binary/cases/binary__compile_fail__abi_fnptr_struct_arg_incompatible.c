typedef int (*op_fn)(int);

struct payload {
    int value;
};

static int invoke(op_fn fn, int x) {
    return fn(x);
}

static int identity(int x) {
    return x;
}

int main(void) {
    struct payload p = {7};
    return invoke(identity, 4) + invoke(p, 5);
}
