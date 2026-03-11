struct S {
    int x;
};

int main(void) {
    struct S s = {1};
    return +s;
}
