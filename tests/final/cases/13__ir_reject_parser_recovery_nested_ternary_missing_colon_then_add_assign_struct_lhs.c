struct Pair {
    int a;
};

int main(void) {
    int a = 1;
    int b = 2;
    int c = 3;
    int broken = a ? (b ? 4 5) : c;
    struct Pair p = {1};
    p += 1;
    return 0;
}
