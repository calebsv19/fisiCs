int overflow_path(int x) {
    int y = x + 1;
    if (y < x) {
        return y ^ x;
    }
    return y;
}

int main(void) {
    volatile int seed = 2147483647;
    return overflow_path(seed);
}
