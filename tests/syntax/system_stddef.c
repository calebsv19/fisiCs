#include <stddef.h>

struct Pair { int a; double b; };

int main(void) {
    size_t n = sizeof(struct Pair);
    ptrdiff_t delta = (char*)&((struct Pair*)0)->b - (char*)&((struct Pair*)0)->a;
    void* p = NULL;
    return (int)(n + delta + (p == NULL));
}
