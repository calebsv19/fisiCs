#include <stdlib.h>

int main(void) {
    void* p = malloc(21);
    if (!p) return 2;
    return 0;
}
