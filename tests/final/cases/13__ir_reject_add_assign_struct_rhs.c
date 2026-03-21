struct Pair {
    int a;
};

int main(void) {
    int x = 1;
    struct Pair s = {2};
    x += s;
    return x;
}
