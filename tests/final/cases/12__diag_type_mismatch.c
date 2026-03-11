struct Pair {
    int left;
    int right;
};

int main(void) {
    int x = 0;
    struct Pair p = {1, 2};
    x = p;
    return x;
}
