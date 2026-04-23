int qe62_should_fail(unsigned step) {
    if ((step % 8u) == 0u) {
        return 1;
    }
    if ((step % 13u) == 4u) {
        return 1;
    }
    return 0;
}
