struct S {
    char c;
    int i;
    short s;
};

union U {
    int i;
    char c[4];
};

int main(void) {
    struct S s;
    int ok = 1;

    ok = ok && ((char *)&s.c == (char *)&s);
    ok = ok && ((char *)&s.i == (char *)&s + 4);
    ok = ok && ((char *)&s.s == (char *)&s + 8);
    ok = ok && (sizeof(struct S) == 12);
    ok = ok && (sizeof(union U) == 4);

    return ok ? 0 : 1;
}
