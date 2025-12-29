#include <stdarg.h>

int vfun(int count, ...) {
    va_list ap;
    va_start(ap, count);
    va_end(ap);
    return count;
}

int main(void) {
    float f = 1.5f;
    char c = 3;
    return vfun(1, f, c);
}
