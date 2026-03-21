struct S {
    int x;
};

int main(void) {
    int cond = 1;
    int value = 4;
    int* p = &value;
    struct S s = {7};
    cond ? (p << 1) : s;
    return 0;
}
