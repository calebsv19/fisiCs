struct S { int a; int b; };
int main(void) {
    struct S s = {1, 2};
    return s.a + s.b;
}
