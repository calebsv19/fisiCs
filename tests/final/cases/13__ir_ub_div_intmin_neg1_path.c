int ub_divide(int x) {
    int y = x / -1;
    if (y < 0) {
        return y;
    }
    return y + 1;
}

int main(void) {
    volatile int seed = (int)0x80000000u;
    return ub_divide(seed);
}
