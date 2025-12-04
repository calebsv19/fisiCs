struct BF {
    int a:3;
    unsigned b:33;      // too wide for unsigned int (assume 32)
    unsigned :0;        // ok
    int named_zero:0;   // invalid: zero width but named
    float f:2;          // invalid: non-integral base
    int neg:-1;         // invalid: negative width
    enum Shade { SHADE_A, SHADE_B } shade:2; // ok
};

int main(void) {
    struct BF bf = {0};
    return bf.a;
}
