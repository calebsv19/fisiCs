struct Bits {
    unsigned a : 3;
    signed   b : 5;
    unsigned : 0;
    unsigned c : 4;
};

int update(struct Bits s, int x) {
    s.a = (unsigned)x;
    s.b += 2;
    s.c = (unsigned)(s.a + s.b);
    return s.c;
}

int main(void) {
    struct Bits b = { .a = 1, .b = -1, .c = 0 };
    return update(b, 5);
}
