int overflow_mul(int x) {
    int y = x * 3;
    if (y / 3 != x) {
        return y;
    }
    return x;
}

int main(void) {
    volatile int seed = 1073741824;
    return overflow_mul(seed);
}
