int ub_chain(int x) {
    int a = x + x;
    int b = a - x;
    if (a < b) {
        return a;
    }
    return b;
}

int main(void) {
    volatile int seed = 1073741824;
    return ub_chain(seed);
}
