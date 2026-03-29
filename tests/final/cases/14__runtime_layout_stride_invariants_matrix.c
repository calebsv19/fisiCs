#include <stddef.h>
#include <stdio.h>

struct A {
    char c;
    int i;
    double d;
};

struct B {
    struct A arr[3];
    long long z;
};

int main(void) {
    struct B b;
    int k1 = (offsetof(struct A, i) >= sizeof(char));
    int k2 = (offsetof(struct A, d) >= offsetof(struct A, i) + sizeof(int));
    int k3 = (sizeof(struct A) >= offsetof(struct A, d) + sizeof(double));
    int k4 = (((char*)&b.arr[1] - (char*)&b.arr[0]) == (ptrdiff_t)sizeof(struct A));
    int k5 = (offsetof(struct B, z) >= sizeof(struct A) * 3u);
    printf("%d %d %d %d %d\n", k1, k2, k3, k4, k5);
    return 0;
}
