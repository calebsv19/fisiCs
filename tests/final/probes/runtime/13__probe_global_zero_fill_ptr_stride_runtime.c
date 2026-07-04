#include <stdio.h>

typedef struct {
    int base;
    int extra[3];
} Row;

static Row rows[3] = {
    {1, {2}},
    {4},
    {0, {7, 8}},
};

int main(void) {
    Row *start = rows;
    Row *end = rows + 3;
    int total = 0;

    for (Row *p = start; p != end; ++p) {
        int idx = (int)(p - start);
        int *slot = (idx & 1) ? &p->extra[2] : &p->extra[1];

        total += p->base + *slot;
        if (idx == 0) {
            continue;
        }
        total += p[-1].extra[idx - 1];
    }

    printf("%d %d %d %d\n", (int)(end - start), rows[0].extra[2], rows[1].extra[1], total);
    return 0;
}
