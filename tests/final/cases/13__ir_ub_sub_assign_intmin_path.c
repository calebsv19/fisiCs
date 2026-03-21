int ub_sub_assign(int x) {
    x -= 1;
    if (x > 0) {
        return x;
    }
    return x - 1;
}

int main(void) {
    volatile int seed = (int)0x80000000u;
    return ub_sub_assign(seed);
}
