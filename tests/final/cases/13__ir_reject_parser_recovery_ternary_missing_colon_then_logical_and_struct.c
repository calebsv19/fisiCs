struct S {
    int x;
};

int main(void) {
    int keep = 1;
    int broken = keep ? 2 3;
    struct S s = {1};
    return s && 1;
}
