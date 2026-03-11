int inc_register(int x) {
    register int r = x;
    r = r + 1;
    return r;
}
