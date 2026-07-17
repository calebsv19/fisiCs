#include <stdio.h>

typedef struct {
    volatile int count;
    const int limit;
    int data[2];
} Sensor;

typedef struct {
    Sensor sensors[2];
    int scale;
} Bank;

int main(void) {
    Bank banks[2] = {
        {{{3, 5, {6, 7}}, {7, 11, {8, 9}}}, 13},
        {{{13, 17, {10, 11}}, {17, 19, {12, 13}}}, 23},
    };

    int pick = banks[1].sensors[1].limit > banks[0].scale;
    volatile int *counter = pick ? &banks[1].sensors[1].count : &banks[0].sensors[0].count;
    const int *limit = pick ? &banks[1].sensors[1].limit : &banks[0].sensors[0].limit;
    *counter += *limit;

    int *data = pick ? &banks[0].sensors[1].data[0] : &banks[1].sensors[0].data[1];
    *data += *counter + *limit;

    int total = *data + *counter + *limit + banks[0].sensors[1].count;
    printf("%d %d %d %d\n", *data, *counter, *limit, total);
    return 0;
}
