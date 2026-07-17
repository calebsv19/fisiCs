#include <stdio.h>

typedef struct {
    int sample[3];
    int bias;
} Cell;

typedef struct {
    Cell cells[2];
    int tag;
} Row;

int main(void) {
    Row rows[2] = {
        {{{{2, 4, 6}, 3}, {{8, 10, 12}, 5}}, 7},
        {{{{14, 16, 18}, 11}, {{20, 22, 24}, 13}}, 17},
    };

    int pick = rows[1].cells[0].sample[1] > rows[0].cells[1].sample[0];
    Cell (*selected_cells)[2] = pick ? &rows[1].cells : &rows[0].cells;
    int *slot = &(*selected_cells)[pick ? 0 : 1].sample[pick ? 2 : 0];

    int before = (*slot)++;
    (*selected_cells)[pick ? 1 : 0].bias += before + rows[pick].tag;

    int (*selected_samples)[3] = pick ? &rows[0].cells[1].sample : &rows[1].cells[0].sample;
    ++(*selected_samples)[pick ? 1 : 2];

    int total = *slot + rows[1].cells[1].bias + rows[0].cells[1].sample[1] + before;
    printf("%d %d %d %d\n", *slot, rows[1].cells[1].bias, rows[0].cells[1].sample[1], total);
    return 0;
}
