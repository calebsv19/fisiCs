struct Pair {
    int a;
};

int main(void) {
    struct Pair s = {1};
    s += 1;
    return 0;
}
