struct Pair {
    int x;
    int y;
};

int main(void) {
    int v = 5;
    struct Pair p = (struct Pair)v;
    return p.x;
}
