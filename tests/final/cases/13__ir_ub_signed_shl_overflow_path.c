int ub_shift_left(int x) {
    int y = x << 3;
    if (y < 0) {
        return y;
    }
    return x;
}

int main(void) {
    volatile int seed = 0x20000000;
    return ub_shift_left(seed);
}
