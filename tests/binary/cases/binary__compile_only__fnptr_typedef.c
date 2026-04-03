typedef int (*binary_fn_t)(int);

int identity(int x) {
    return x;
}

binary_fn_t select_identity(void) {
    return identity;
}

