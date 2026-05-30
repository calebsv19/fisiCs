#define NDEBUG 1

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int compare_ints(const void *lhs, const void *rhs) {
    const int *a = (const int *)lhs;
    const int *b = (const int *)rhs;
    return (*a > *b) - (*a < *b);
}

int main(void) {
    int values[4];
    int key = 3;
    int *found = 0;
    char joined[16];

    values[0] = 7;
    values[1] = 1;
    values[2] = 9;
    values[3] = 3;

    qsort(values, 4, sizeof(values[0]), compare_ints);
    found = (int *)bsearch(&key, values, 4, sizeof(values[0]), compare_ints);
    assert(found != 0);

    snprintf(joined, sizeof(joined), "%d,%d,%d,%d", values[0], values[1], values[2], values[3]);
    if (!found || *found != 3 || strcmp(joined, "1,3,7,9") != 0) {
        return 1;
    }

    printf("sorted=%s found=%d\n", joined, *found);
    return 0;
}
