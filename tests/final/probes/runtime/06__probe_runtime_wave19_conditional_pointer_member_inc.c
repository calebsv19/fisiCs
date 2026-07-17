#include <stdio.h>

typedef struct {
    int value;
    int extra[2];
} Cell;

typedef struct {
    Cell cells[2];
    volatile int writes;
} Box;

int main(void) {
    Box boxes[2] = {
        {{{3, {5, 7}}, {11, {13, 17}}}, 19},
        {{{23, {29, 31}}, {37, {41, 43}}}, 47},
    };

    int pick = boxes[0].cells[1].extra[1] < boxes[1].cells[0].extra[0];
    Box *selected = pick ? &boxes[0] : &boxes[1];
    int before = selected->cells[pick ? 1 : 0].value++;
    selected->cells[pick ? 1 : 0].extra[pick ? 0 : 1] += before;
    selected->writes += selected->cells[pick ? 1 : 0].extra[pick ? 0 : 1];

    int total = before + selected->cells[pick ? 1 : 0].value +
                selected->cells[pick ? 1 : 0].extra[pick ? 0 : 1] + selected->writes;
    printf("%d %d %d %d\n", before, selected->cells[pick ? 1 : 0].value,
           selected->cells[pick ? 1 : 0].extra[pick ? 0 : 1], total);
    return 0;
}
