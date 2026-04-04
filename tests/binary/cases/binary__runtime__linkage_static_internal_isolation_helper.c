static int helper_internal = 100;

int helper_internal_bump(int delta) {
    helper_internal += delta;
    return helper_internal;
}

int helper_internal_get(void) {
    return helper_internal;
}
