#include <stdlib.h>

int main(void) {
    void* leaked = malloc(21);
    if (!leaked) return 2;

    /* Intentionally leaked so the memory-check overlay has something visible. */
    return 0;
}
