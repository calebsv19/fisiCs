#include <stddef.h>
#include <stdio.h>

struct S {
    char c;
    int i;
    short s;
    double d;
};

int main(void) {
    printf(
        "%zu %zu %zu %zu %zu\n",
        sizeof(struct S),
        offsetof(struct S, i),
        offsetof(struct S, s),
        offsetof(struct S, d),
        (size_t)(offsetof(struct S, d) % sizeof(double))
    );
    return 0;
}
