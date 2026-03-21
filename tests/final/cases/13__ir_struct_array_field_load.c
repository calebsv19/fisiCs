struct S {
    int a[3];
};

int main(void) {
    struct S s = {{1, 2, 3}};
    return s.a[2];
}
