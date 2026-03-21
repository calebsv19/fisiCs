int ub_shift_negative_count(int x, int s) {
    int y = x << s;
    if (y < x) {
        return y;
    }
    return x;
}

int main(void) {
    volatile int seed = 3;
    volatile int shift = -1;
    return ub_shift_negative_count(seed, shift);
}
