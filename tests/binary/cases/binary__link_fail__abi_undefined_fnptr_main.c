typedef int (*op_fn)(int);

op_fn abi_pick_fn(void);

int main(void) {
    op_fn fn = abi_pick_fn();
    return fn(7);
}
