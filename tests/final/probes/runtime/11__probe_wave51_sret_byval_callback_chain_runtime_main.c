#include <stdio.h>

struct wave51_chain_cell {
    int lane[4];
};

struct wave51_chain_payload {
    struct wave51_chain_cell head;
    struct wave51_chain_cell tail;
    long checksum;
    int rounds;
};

typedef struct wave51_chain_cell (*wave51_chain_cb)(struct wave51_chain_cell item, int salt);

struct wave51_chain_payload wave51_sret_byval_callback_chain(struct wave51_chain_cell seed,
                                                             wave51_chain_cb cb,
                                                             int rounds,
                                                             int salt);

static struct wave51_chain_cell wave51_chain_twist(struct wave51_chain_cell item, int salt) {
    struct wave51_chain_cell out;
    int i;
    for (i = 0; i < 4; i++) {
        out.lane[i] = item.lane[3 - i] + salt * (i + 1) + item.lane[i];
    }
    return out;
}

int main(void) {
    struct wave51_chain_cell seed = {{3, 5, 7, 11}};
    struct wave51_chain_payload got = wave51_sret_byval_callback_chain(seed, wave51_chain_twist, 3, 4);
    printf("%d %d %d %d %ld %d\n",
           got.head.lane[0],
           got.head.lane[3],
           got.tail.lane[1],
           got.tail.lane[2],
           got.checksum,
           got.rounds);
    return 0;
}
