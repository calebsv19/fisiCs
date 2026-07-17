#include <stdio.h>

typedef struct {
    volatile int flag;
    int payload[3];
} Unit;

int main(void) {
    Unit units[2] = {
        {2, {3, 5, 7}},
        {11, {13, 17, 19}},
    };

    int pick = units[0].payload[2] < units[1].payload[0];
    volatile int *selected_flag = pick ? &units[1].flag : &units[0].flag;
    *selected_flag += units[0].flag;

    const Unit *view = pick ? (const Unit *)&units[1] : (const Unit *)&units[0];
    const int *read_payload = pick ? &view->payload[2] : &view->payload[0];
    int *write_payload = pick ? &units[0].payload[1] : &units[1].payload[1];
    *write_payload += *read_payload + view->flag;

    int total = units[0].payload[1] + units[1].flag + *read_payload + units[0].flag;
    printf("%d %d %d %d\n", units[0].payload[1], units[1].flag, *read_payload, total);
    return 0;
}
