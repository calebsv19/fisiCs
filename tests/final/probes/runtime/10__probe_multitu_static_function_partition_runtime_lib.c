static int helper(void) {
    return 11;
}

int call_local_helper(void) {
    return helper();
}
