struct S {
    int x;
};

int main(void) {
    int keep = 1;
    int broken = keep ? 2 3;
    struct S a = {1};
    struct S b = {2};
    return a == b;
}
