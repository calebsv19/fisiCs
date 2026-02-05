enum ConvCheck {
    ok = ((1u + -2) > 0) ? 1 : -1
};

struct S {
    int b:3;
    const int c;
};

int main(void) {
    struct S s = {0, 0};
    int *pb = &s.b;
    s.c = 1;
    int *p = 0;
    if (p == 1) {
        return 1;
    }
    int arr[ok];
    return arr[0];
}
