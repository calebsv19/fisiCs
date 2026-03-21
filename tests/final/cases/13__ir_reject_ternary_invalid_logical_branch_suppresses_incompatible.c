struct S {
    int x;
};

int main(void) {
    int cond = 1;
    struct S s = {1};
    cond ? (s && 1) : s;
    return 0;
}
