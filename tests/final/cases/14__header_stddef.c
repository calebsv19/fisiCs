#include <stddef.h>
int main(void) {
    size_t n = 4;
    int *p = NULL;
    return (int)n + (p == NULL);
}
