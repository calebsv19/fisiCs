int qb15_should_fail(unsigned step) {
    if ((step % 7u) == 0u) {
        return 1;
    }
    if ((step % 13u) == 5u) {
        return 1;
    }
    return 0;
}
