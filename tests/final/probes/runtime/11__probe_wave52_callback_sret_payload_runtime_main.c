#include <stdio.h>

struct wave52_sret_cell {
    int lane[4];
};

struct wave52_sret_payload {
    struct wave52_sret_cell first;
    struct wave52_sret_cell last;
    long sum;
    int tag;
};

typedef struct wave52_sret_payload (*wave52_sret_cb)(struct wave52_sret_cell cell, int salt);

struct wave52_sret_payload wave52_callback_sret_payload(struct wave52_sret_cell seed,
                                                        wave52_sret_cb cb,
                                                        int rounds,
                                                        int salt);

static struct wave52_sret_payload wave52_build_payload(struct wave52_sret_cell cell, int salt) {
    struct wave52_sret_payload out;
    int i;

    out.first = cell;
    out.sum = salt;
    out.tag = salt & 7;
    for (i = 0; i < 4; i++) {
        out.last.lane[i] = cell.lane[i] + salt * (i + 1);
        out.sum += (long)out.last.lane[i] * (i + 2);
    }

    return out;
}

int main(void) {
    struct wave52_sret_cell seed = {{4, 6, 8, 10}};
    struct wave52_sret_payload got = wave52_callback_sret_payload(seed, wave52_build_payload, 3, 5);
    printf("%d %d %d %d %ld %d\n",
           got.first.lane[0],
           got.first.lane[2],
           got.last.lane[1],
           got.last.lane[3],
           got.sum,
           got.tag);
    return 0;
}
