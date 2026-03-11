struct S {
    int x;
};

int main(void) {
    struct S a = {1};
    struct S b = {2};
    return a < b;
}
