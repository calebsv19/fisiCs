struct Bits { unsigned a:3; };
int main(void) {
    struct Bits b = {0};
    int *p = &b.a;
    register int r = 0;
    int *q = &r;
    int *t = &(1 + 2);
    return (p != q) + (t != 0);
}
