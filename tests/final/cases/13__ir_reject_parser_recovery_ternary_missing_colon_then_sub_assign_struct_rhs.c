struct Pair {
    int a;
};

int main(void) {
    int keep = 1;
    int broken = keep ? 2 3;
    int x = 1;
    struct Pair s = {2};
    x -= s;
    return x;
}
