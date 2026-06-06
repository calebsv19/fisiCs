#include <stdio.h>
#include <stdlib.h>

struct Row {
    int key;
    int ordinal;
};

static int compare_row(const void *left, const void *right) {
    const struct Row *a = (const struct Row *)left;
    const struct Row *b = (const struct Row *)right;
    if (a->key != b->key) {
        return a->key < b->key ? -1 : 1;
    }
    if (a->ordinal != b->ordinal) {
        return a->ordinal < b->ordinal ? -1 : 1;
    }
    return 0;
}

int main(void) {
    struct Row rows[] = {
        {3, 1},
        {1, 4},
        {2, 2},
        {1, 1},
        {3, 0},
        {2, 5},
    };
    struct Row needle = {2, 5};
    struct Row missing = {4, 0};
    struct Row empty_probe = {1, 1};
    size_t count = sizeof(rows) / sizeof(rows[0]);
    int checksum = 0;

    qsort(rows, count, sizeof(rows[0]), compare_row);
    for (size_t i = 0; i < count; ++i) {
        checksum += (int)(i + 1U) * (rows[i].key * 10 + rows[i].ordinal);
    }

    struct Row *found = (struct Row *)bsearch(&needle, rows, count, sizeof(rows[0]), compare_row);
    struct Row *not_found = (struct Row *)bsearch(&missing, rows, count, sizeof(rows[0]), compare_row);
    struct Row *empty = (struct Row *)bsearch(&empty_probe, rows, 0, sizeof(rows[0]), compare_row);
    long found_index = found ? (long)(found - rows) : -1L;

    printf("stdlib-sort-search checksum=%d found=%ld missing=%d empty=%d first=%d last=%d\n",
           checksum,
           found_index,
           not_found == NULL,
           empty == NULL,
           rows[0].key * 10 + rows[0].ordinal,
           rows[count - 1U].key * 10 + rows[count - 1U].ordinal);

    return checksum == 541 && found_index == 3L && not_found == NULL &&
                   empty == NULL && rows[0].key == 1 && rows[0].ordinal == 1 &&
                   rows[count - 1U].key == 3 && rows[count - 1U].ordinal == 1
               ? 0
               : 1;
}
