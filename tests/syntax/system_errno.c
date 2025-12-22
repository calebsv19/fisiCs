#include <errno.h>

int main(void) {
    errno = EINVAL;
    return errno == EINVAL ? 0 : 1;
}
