int ub_negate(int x) {
    int y = -x;
    if (y == x) {
        return y;
    }
    return y + 1;
}

int main(void) {
    volatile int seed = (int)0x80000000u;
    return ub_negate(seed);
}
