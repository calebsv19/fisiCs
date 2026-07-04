#include <stdio.h>

typedef struct {
    int base;
    int offsets[3];
} Row;

static int project_row(const Row *row, const int *slot) {
    return row->base + *slot;
}

static int scan_row(const Row *start, const Row *cursor) {
    int idx = (int)(cursor - start);
    const int *slot = (idx & 1) ? &cursor->offsets[2] : &cursor->offsets[0];
    return project_row(cursor, slot) + idx;
}

int main(void) {
    Row rows[4] = {
        {2, {1, 0, 5}},
        {4, {2, 6, 8}},
        {7, {3, 9, 1}},
        {1, {4, 5, 7}},
    };
    int total = 0;

    for (Row *p = rows + 1; p < rows + 4; ++p) {
        int idx = (int)(p - rows);
        total += scan_row(rows, p);
        total += idx;
        total += p[-1].offsets[idx - 1];
    }

    printf("%d %d %d %d\n", (int)((rows + 3) - rows), rows[1].offsets[1], rows[2].offsets[2], total);
    return 0;
}
