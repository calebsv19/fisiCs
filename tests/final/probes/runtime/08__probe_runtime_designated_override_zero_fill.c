#include <stdio.h>

struct Record {
    int id;
    int flags;
    int values[4];
};

static struct Record build_record(void) {
    struct Record r = {
        .values[2] = 7,
        .id = 3,
        .values[0] = 1,
        .values[2] = 9,
    };
    return r;
}

int main(void) {
    struct Record r = build_record();
    printf("%d %d %d %d %d %d\n",
           r.id,
           r.flags,
           r.values[0],
           r.values[1],
           r.values[2],
           r.values[3]);
    return 0;
}
