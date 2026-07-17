#include <stdio.h>

typedef struct {
    int slots[3];
    volatile int marker;
} Row;

typedef struct {
    Row rows[2];
    int bias;
} Table;

int main(void) {
    Table tables[2] = {
        {{{{2, 4, 6}, 8}, {{10, 12, 14}, 16}}, 18},
        {{{{20, 22, 24}, 26}, {{28, 30, 32}, 34}}, 36},
    };

    int pick = tables[1].rows[0].slots[2] > tables[0].rows[1].slots[1];
    Row (*selected_rows)[2] = pick ? &tables[1].rows : &tables[0].rows;
    int *slot = &(*selected_rows)[pick ? 0 : 1].slots[pick ? 1 : 2];
    *slot += tables[pick ? 0 : 1].bias;
    ++(*selected_rows)[pick ? 1 : 0].marker;
    (*selected_rows)[pick ? 1 : 0].slots[0] += (*selected_rows)[pick ? 1 : 0].marker;

    int total = *slot + (*selected_rows)[1].marker + (*selected_rows)[1].slots[0] + tables[0].bias;
    printf("%d %d %d %d\n", *slot, (*selected_rows)[1].marker, (*selected_rows)[1].slots[0], total);
    return 0;
}
