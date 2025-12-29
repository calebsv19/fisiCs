// Ensure unsigned right shift lowers to lshr in IR.
unsigned foo(unsigned x) {
    return x >> 1;
}

int main(void) {
    return (int)foo(2u);
}
