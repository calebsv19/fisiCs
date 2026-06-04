#include <errno.h>

int wave27_errno_macro_surface(void) {
    int *errno_ptr = &errno;
    int total = EDOM + ERANGE;

    errno = 0;
    *errno_ptr = total;
    return errno;
}

int main(void) {
    return 0;
}
