struct S {
    int a;
    int b;
    int c;
};

struct S s = { .b = 7 };

int main(void) {
    return s.a + s.b + s.c;
}
