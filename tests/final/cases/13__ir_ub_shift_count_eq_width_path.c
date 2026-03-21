int ub_shift_eq_width(int x, int s) {
    int y = x << s;
    if (y == 0) {
        return x;
    }
    return y;
}

int main(void) {
    volatile int seed = 1;
    volatile int shift = (int)(sizeof(int) * 8);
    return ub_shift_eq_width(seed, shift);
}
