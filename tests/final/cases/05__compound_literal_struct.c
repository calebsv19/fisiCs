struct Pair {
    int a;
    int b;
};

int main(void) {
    struct Pair p = (struct Pair){10, 20};
    return p.a + p.b;
}
