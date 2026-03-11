#include <stdio.h>

struct Big {
    int a0;
    int a1;
    int a2;
    int a3;
    int a4;
    int a5;
    int a6;
    int a7;
    int a8;
    int a9;
    int a10;
    int a11;
    int a12;
    int a13;
    int a14;
    int a15;
    int a16;
    int a17;
    int a18;
    int a19;
    int a20;
    int a21;
    int a22;
    int a23;
    int a24;
    int a25;
    int a26;
    int a27;
    int a28;
    int a29;
    int a30;
    int a31;
};

int main(void) {
    struct Big b = {0};
    b.a0 = 1;
    b.a15 = 16;
    b.a31 = 32;
    printf("%d %zu\n", b.a0 + b.a15 + b.a31, sizeof(struct Big) / sizeof(int));
    return 0;
}
