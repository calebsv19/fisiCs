#include <stdio.h>

typedef struct {
    volatile int counter;
    int values[2];
} Slot;

typedef struct {
    Slot slots[2];
    int bias;
} Box;

int main(void) {
    Box boxes[2] = {
        {{{3, {5, 7}}, {11, {13, 17}}}, 19},
        {{{23, {29, 31}}, {37, {41, 43}}}, 47},
    };

    int pick = boxes[0].slots[1].values[1] < boxes[1].slots[0].values[0];
    Slot *selected = pick ? &boxes[1].slots[0] : &boxes[0].slots[1];
    int before = selected->counter++;
    selected->values[pick ? 1 : 0] += before + boxes[pick ? 0 : 1].bias;
    selected->counter += selected->values[pick ? 1 : 0];

    int total = before + selected->counter + selected->values[pick ? 1 : 0];
    printf("%d %d %d\n", before, selected->counter, total);
    return 0;
}
