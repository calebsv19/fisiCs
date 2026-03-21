int ub_modulo(int x) {
    int y = x % -1;
    if (y != 0) {
        return y;
    }
    return x;
}

int main(void) {
    volatile int seed = (int)0x80000000u;
    return ub_modulo(seed);
}
