struct Pair {
    int a;
};

int main(void) {
    struct Pair p = {1};
    p ^= 1;
    return 0;
}
