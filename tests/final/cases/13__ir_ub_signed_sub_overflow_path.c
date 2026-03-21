int overflow_sub(int x) {
    int y = x - 2;
    if (y > x) {
        return y;
    }
    return x - y;
}

int main(void) {
    volatile int seed = -2147483647;
    return overflow_sub(seed);
}
