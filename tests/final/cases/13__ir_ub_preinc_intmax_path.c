int ub_preinc(int x) {
    int y = ++x;
    if (y < 0) {
        return y;
    }
    return x;
}

int main(void) {
    volatile int seed = 2147483647;
    return ub_preinc(seed);
}
