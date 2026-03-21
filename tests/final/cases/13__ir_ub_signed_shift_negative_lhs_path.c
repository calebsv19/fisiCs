int ub_shift_neg(int x) {
    int y = x << 1;
    if (y < x) {
        return y;
    }
    return x;
}

int main(void) {
    volatile int seed = -3;
    return ub_shift_neg(seed);
}
