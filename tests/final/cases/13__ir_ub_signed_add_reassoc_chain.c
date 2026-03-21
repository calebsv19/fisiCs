int ub_reassoc(int x) {
    int a = x + 1000000000;
    int b = a + 1000000000;
    int c = b - 1000000000;
    if (c < x) {
        return c;
    }
    return b;
}

int main(void) {
    volatile int seed = 1500000000;
    return ub_reassoc(seed);
}
