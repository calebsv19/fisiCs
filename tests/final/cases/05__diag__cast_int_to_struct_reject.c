struct S {
    int x;
};

int main(void) {
    struct S s = (struct S)1;
    return s.x;
}
