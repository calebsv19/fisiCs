int const_mutation(void) {
    const int c = 4;
    int x = c;
    c = x + 1;
    return x;
}
