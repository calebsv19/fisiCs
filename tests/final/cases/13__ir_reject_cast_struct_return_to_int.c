struct S {
    int x;
};

struct S make_s(void) {
    struct S s = {1};
    return s;
}

int main(void) {
    return (int)make_s();
}
