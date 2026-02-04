struct S { int b; };

#define CAT(a, b) a ## b
#define STR(a) #a

int vfunc(int a, ...);

int main(void) {
    int a = 1 >> 2;
    a >>= 1;
    int ab = 0;
    int cat = CAT(a, b);
    const char* s = STR(hello);
    struct S s1 = { 3 };
    struct S* p = &s1;
    int d = p->b;
    return a + ab + cat + d + (int)s[0];
}
