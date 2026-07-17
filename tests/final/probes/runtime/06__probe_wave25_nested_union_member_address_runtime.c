#include <stdio.h>

struct Wave25Record {
    int total;
    int lanes[2];
};

union Wave25State {
    struct Wave25Record record;
    unsigned raw[3];
};

int main(void) {
    union Wave25State states[2] = {
        {{7, {11, 13}}},
        {{17, {19, 23}}},
    };
    int pick = states[1].record.lanes[0] > states[0].record.lanes[1];
    union Wave25State *selected = pick ? &states[1] : &states[0];
    int *total = &selected->record.total;
    int *lane = &selected->record.lanes[pick ? 1 : 0];

    *lane += *total;
    *total += selected->record.lanes[0];
    printf("%d %d %d\n", *total, *lane, selected->record.lanes[0]);
    return 0;
}
