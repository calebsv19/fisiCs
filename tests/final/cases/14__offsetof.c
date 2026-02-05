#include <stddef.h>

struct S {
    char a;
    int b;
};

int main(void) {
    return (int)offsetof(struct S, b);
}
