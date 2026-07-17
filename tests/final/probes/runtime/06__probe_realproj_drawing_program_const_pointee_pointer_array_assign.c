#include <stddef.h>

int main(void) {
    int first = 11;
    int second = 29;
    const int *slots[4] = { 0 };
    size_t cursor = 0u;

    slots[cursor++] = &first;
    slots[cursor++] = &second;

    return cursor == 2u && *slots[0] == 11 && *slots[1] == 29 ? 0 : 1;
}
