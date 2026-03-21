#include <stddef.h>
#include <stdio.h>

typedef struct {
    char c;
    long double ld;
    int tail;
} Box;

int main(void) {
    size_t off_ld = offsetof(Box, ld);
    size_t off_tail = offsetof(Box, tail);
    size_t align_like = offsetof(struct {
        char c;
        long double x;
    }, x);
    size_t size_box = sizeof(Box);

    int ok1 = (off_ld == align_like);
    int ok2 = (off_tail >= off_ld + sizeof(long double));
    int ok3 = (size_box >= off_tail + sizeof(int));
    int ok4 = (size_box % align_like) == 0;

    printf("%d %d %d %d\n", ok1, ok2, ok3, ok4);
    return 0;
}
