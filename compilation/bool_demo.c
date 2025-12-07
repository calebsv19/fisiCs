#include "stdbool.h"

// Minimal write prototype to avoid depending on stdio.
long write(int fd, const void* buf, unsigned long count);

int main(void) {
    bool ok = true;
    const char* msg_true = "bool demo: true\n";
    const char* msg_false = "bool demo: false\n";
    const char* msg = ok ? msg_true : msg_false;
    unsigned long len = ok ? 15 : 16; // literal lengths without null
    write(1, msg, len);
    return ok ? 0 : 1;
}
