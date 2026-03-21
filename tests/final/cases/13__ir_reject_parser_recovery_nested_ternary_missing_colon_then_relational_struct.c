struct S {
    int x;
};

int main(void) {
    int a = 1;
    int b = 2;
    int c = 3;
    int broken = a ? (b ? 4 5) : c;
    struct S lhs = {1};
    struct S rhs = {2};
    return lhs < rhs;
}
