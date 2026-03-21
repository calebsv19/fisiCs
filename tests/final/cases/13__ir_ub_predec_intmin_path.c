int ub_predec(int x) {
    int y = --x;
    if (y > 0) {
        return y;
    }
    return x;
}

int main(void) {
    volatile int seed = (int)0x80000000u;
    return ub_predec(seed);
}
