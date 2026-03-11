void mutate_const_pointer(void) {
    int x = 0;
    int * const p = &x;
    p = 0;
}
