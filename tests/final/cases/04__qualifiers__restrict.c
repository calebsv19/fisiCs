int add_restrict(int *restrict out, const int *restrict a, const int *restrict b) {
    *out = *a + *b;
    return *out;
}
