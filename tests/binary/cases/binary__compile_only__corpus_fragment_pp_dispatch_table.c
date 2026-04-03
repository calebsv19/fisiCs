#define OP_LIST(X) \
    X(add, +)      \
    X(sub, -)      \
    X(xor, ^)

#define DECL_ENUM(name, sym) OP_##name,
typedef enum {
    OP_LIST(DECL_ENUM)
    OP_COUNT
} OpCode;
#undef DECL_ENUM

typedef int (*op_fn)(int a, int b);

static int op_add(int a, int b) { return a + b; }
static int op_sub(int a, int b) { return a - b; }
static int op_xor(int a, int b) { return a ^ b; }

static op_fn dispatch(OpCode op) {
    static op_fn table[OP_COUNT] = {op_add, op_sub, op_xor};
    if ((unsigned)op >= (unsigned)OP_COUNT) return (op_fn)0;
    return table[(unsigned)op];
}

int eval_dispatch(OpCode op, int a, int b) {
    op_fn fn = dispatch(op);
    if (!fn) return 0;
    return fn(a, b);
}
