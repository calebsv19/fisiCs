typedef int (*op_fn)(int);

static int invoke(op_fn fn, int x) { return fn(x); }
static int identity(int x) { return x; }

int main(void) {
    return invoke(identity, 4) + invoke(1, 5);
}
