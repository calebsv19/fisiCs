typedef int I;
typedef unsigned long ULong;

I add(I a, I b) {
    return a + b;
}

int main(void) {
    ULong v = 42;
    return add((I)v, 1);
}
