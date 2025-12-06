// Bare struct definition with trailing semicolon, then use in a function.
struct S { int a; int b; };

int main(void) {
    struct S t = (struct S){ .a = 1, .b = 2 };
    return t.a + t.b;
}
