#include <stdio.h>

typedef union {
    struct {
        int row[2];
        int weight;
    } matrix;
    int lane[3];
} PayloadArm;

typedef struct {
    int tag;
    PayloadArm arm;
    int edge[2];
} Payload;

static Payload make_matrix_payload(int seed) {
    Payload p;
    p.tag = 10;
    p.arm.matrix.row[0] = seed + 1;
    p.arm.matrix.row[1] = seed + 4;
    p.arm.matrix.weight = seed * 2;
    p.edge[0] = seed + 8;
    p.edge[1] = seed + 13;
    return p;
}

static Payload make_lane_payload(int seed) {
    Payload p;
    p.tag = 20;
    p.arm.lane[0] = seed - 2;
    p.arm.lane[1] = seed + 6;
    p.arm.lane[2] = seed * 2 + 1;
    p.edge[0] = seed + 21;
    p.edge[1] = seed + 34;
    return p;
}

static Payload choose_ternary_payload(int flag, int seed) {
    Payload matrix = make_matrix_payload(seed);
    Payload lane = make_lane_payload(seed + 1);
    return flag ? matrix : lane;
}

static int payload_score(Payload p) {
    if (p.tag == 10) {
        return p.arm.matrix.row[0] * 5 + p.arm.matrix.row[1] * 3 +
               p.arm.matrix.weight + p.edge[0] - p.edge[1];
    }
    return p.arm.lane[0] * 7 + p.arm.lane[1] * 2 + p.arm.lane[2] +
           p.edge[0] + p.edge[1];
}

int main(void) {
    Payload acc = make_lane_payload(2);
    int checksum = payload_score(acc);

    for (int i = 0; i < 5; ++i) {
        Payload selected = choose_ternary_payload((checksum + i) & 1, i + 3);
        Payload merged = (payload_score(selected) > payload_score(acc)) ? selected : acc;
        Payload copied;
        copied = merged;
        acc = ((copied.edge[0] + i) & 1) ? copied : selected;
        checksum += payload_score(acc) + acc.tag + acc.edge[1];
    }

    printf("%d %d %d %d\n", acc.tag, acc.edge[0], acc.edge[1], checksum);
    return 0;
}
