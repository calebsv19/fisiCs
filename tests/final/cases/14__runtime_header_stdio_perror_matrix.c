#include <errno.h>
#include <stdio.h>

int main(void) {
    int before = 0;
    int after = 0;
    int summary = 0;

    errno = ENOENT;
    before = errno;
    perror("fisics-perror");
    after = errno;
    summary = before * 7 + after * 11 + (before == after ? 13 : 0);

    printf("stdio-perror before=%d after=%d same=%d summary=%d\n",
           before,
           after,
           before == after,
           summary);

    return before == ENOENT && after == ENOENT ? 0 : 1;
}
