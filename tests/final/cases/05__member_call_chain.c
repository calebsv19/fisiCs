struct S { int (*fn)(int); int value; };
int add1(int x) { return x + 1; }
int main(void) {
    struct S s;
    s.fn = add1;
    s.value = 5;
    int out = s.fn(s.value);
    return out;
}
