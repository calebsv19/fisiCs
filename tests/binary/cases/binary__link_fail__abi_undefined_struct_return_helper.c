typedef struct {
    int a;
    int b;
} Pair;

Pair abi_missing_pair_leaf(void);

Pair abi_make_pair(void) {
    return abi_missing_pair_leaf();
}
