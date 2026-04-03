typedef int (*op_fn)(int);

int abi_missing_leaf(int x);

op_fn abi_pick_fn(void) {
    return abi_missing_leaf;
}
