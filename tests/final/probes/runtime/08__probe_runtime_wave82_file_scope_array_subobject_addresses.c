#include <stdio.h>

struct Pair {
    int left;
    int right;
};

struct Row {
    struct Pair pairs[2];
};

static int seed = 3;
static struct Row rows[2] = {
    [0] = { .pairs = { { 5, 7 }, { 11, 13 } } },
    [1] = { .pairs = { { 17, 19 }, { 23, 29 } } },
};

static int *const pointers[4] = {
    &seed,
    &rows[0].pairs[1].right,
    &rows[1].pairs[0].left,
    &rows[1].pairs[1].right,
};

int main(void) {
    *pointers[1] += *pointers[0];
    *pointers[2] += *pointers[1];
    printf("%d %d %d %d\n",
           rows[0].pairs[1].right,
           rows[1].pairs[0].left,
           *pointers[3],
           pointers[2] == &rows[1].pairs[0].left);
    return 0;
}
