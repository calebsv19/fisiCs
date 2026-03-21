struct Pair {
    int a;
};

int main(void) {
    int keep = 1;
    int broken = keep ? 2 3;
    struct Pair p = {1};
    p += 1;
    return 0;
}
