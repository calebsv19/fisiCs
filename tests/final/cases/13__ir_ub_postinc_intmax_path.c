int ub_postinc(int x) {
    int y = x++;
    if (x < y) {
        return x ^ y;
    }
    return x + y;
}

int main(void) {
    volatile int seed = 2147483647;
    return ub_postinc(seed);
}
