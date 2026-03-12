int step(void) {
    int v = 1;
    if (v) {
        goto missing_inner;
    }
    return 0;
}
