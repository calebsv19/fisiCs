typedef int (*op2_fn)(int, int);

static int add_op(int a, int b) { return a + b; }
static int sub_op(int a, int b) { return a - b; }
static int mul_op(int a, int b) { return a * b; }

op2_fn abi_get_op(int id) {
    if (id == 0) return add_op;
    if (id == 1) return sub_op;
    return mul_op;
}

int abi_apply(op2_fn fn, int a, int b) {
    return fn(a, b);
}
