#include <stdio.h>

typedef union {
    struct {
        int row[2];
        int weight;
    } matrix;
    struct {
        int code;
        int value;
        int salt;
    } token;
} Wave43ReturnPayload;

typedef struct {
    int tag;
    Wave43ReturnPayload payload;
    int extra[2];
} Wave43ReturnBundle;

static Wave43ReturnBundle make_matrix(int seed) {
    return (Wave43ReturnBundle){
        1,
        {.matrix = {{seed + 2, seed + 6}, seed % 4 + 3}},
        {seed + 20, seed + 40}
    };
}

static Wave43ReturnBundle make_token(int seed) {
    return (Wave43ReturnBundle){
        2,
        {.token = {seed + 11, seed * 2 + 1, seed - 3}},
        {seed + 30, seed + 50}
    };
}

static Wave43ReturnBundle choose_bundle(int seed, int mode) {
    switch (mode & 3) {
        case 0:
            return make_matrix(seed + 1);
        case 1:
            return make_token(seed + 2);
        case 2:
            return (Wave43ReturnBundle){1, {.matrix = {{seed + 3, seed + 5}, seed + 7}}, {seed + 9, seed + 13}};
        default:
            return (Wave43ReturnBundle){2, {.token = {seed + 4, seed + 8, seed + 12}}, {seed + 16, seed + 18}};
    }
}

static int checksum(Wave43ReturnBundle b) {
    if (b.tag == 1) {
        return b.payload.matrix.row[0] * b.payload.matrix.weight +
               b.payload.matrix.row[1] + b.extra[0] - b.extra[1];
    }
    return b.payload.token.code - b.payload.token.value +
           b.payload.token.salt * 4 + b.extra[0] + b.extra[1];
}

int main(void) {
    Wave43ReturnBundle keep = choose_bundle(4, 0);
    int total = 0;
    int i;

    for (i = 0; i < 10; ++i) {
        Wave43ReturnBundle next = choose_bundle(i + keep.tag, checksum(keep) + i);
        if ((checksum(next) & 1) == 0) {
            keep = next;
        } else {
            keep.extra[i & 1] += i + keep.tag;
        }
        total += checksum(keep);
    }

    printf("%d %d %d %d\n", keep.tag, keep.extra[0], checksum(keep), total);
    return 0;
}
