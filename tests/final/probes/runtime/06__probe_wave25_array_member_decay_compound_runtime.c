#include <stdio.h>

struct Wave25ArrayCell {
    int lanes[3];
    int bias;
};

struct Wave25ArrayBox {
    struct Wave25ArrayCell cells[2];
};

int main(void) {
    struct Wave25ArrayBox boxes[2] = {
        {{{{3, 5, 7}, 11}, {{13, 17, 19}, 23}}},
        {{{{29, 31, 37}, 41}, {{43, 47, 53}, 59}}},
    };
    int pick = boxes[1].cells[0].lanes[2] > boxes[0].cells[1].lanes[1];
    struct Wave25ArrayBox *selected = pick ? &boxes[1] : &boxes[0];
    int (*lanes)[3] = &selected->cells[pick ? 0 : 1].lanes;

    (*lanes)[1] += selected->cells[pick ? 0 : 1].bias;
    (*lanes)[2] += (*lanes)[1];
    printf("%d %d %d\n", (*lanes)[1], (*lanes)[2], selected->cells[pick ? 0 : 1].bias);
    return 0;
}
