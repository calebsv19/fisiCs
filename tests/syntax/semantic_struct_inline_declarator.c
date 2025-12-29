#include <stddef.h>

int main(void) {
    struct Pair { int x; int y; } p = {1, 2};
    return (p.x + p.y) == 3 ? 0 : 1;
}
