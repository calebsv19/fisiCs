#include <stdio.h>

typedef struct {
    volatile int state;
    int value;
} Port;

typedef struct {
    Port ports[2];
    const int scale;
} Device;

int main(void) {
    Device devices[2] = {
        {{{3, 5}, {7, 11}}, 13},
        {{{17, 19}, {23, 29}}, 31},
    };

    int pick = devices[0].ports[1].value < devices[1].ports[0].value;
    volatile int *state = pick ? &devices[1].ports[0].state : &devices[0].ports[0].state;
    *state += devices[pick].scale;

    const Device *view = pick ? &devices[1] : &devices[0];
    int *mutable_value = pick ? &devices[0].ports[1].value : &devices[1].ports[0].value;
    *mutable_value += view->ports[pick ? 1 : 0].state + view->scale;

    int total = *mutable_value + *state + view->scale + devices[0].ports[0].state;
    printf("%d %d %d %d\n", *mutable_value, *state, view->scale, total);
    return 0;
}
