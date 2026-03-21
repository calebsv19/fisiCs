struct S {
    int x;
};

int main(void) {
    struct S cond = {1};
    int value = 2;
    struct S other = {3};
    cond ? value : other;
    return 0;
}
