int ub_add_assign(int x) {
    x += 1;
    if (x < 0) {
        return x;
    }
    return x + 1;
}

int main(void) {
    volatile int seed = 2147483647;
    return ub_add_assign(seed);
}
