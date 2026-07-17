#include <stddef.h>
#include <stdio.h>

union Lane {
    unsigned char bytes[4];
    int scalar;
};

struct Entry {
    unsigned char tag;
    union Lane lane;
};

struct Bundle {
    struct Entry grid[2][2];
    unsigned char end;
};

static struct Entry select_entry(int pick) {
    return pick ? (struct Entry){ .tag = 5, .lane.bytes = { [0] = 7, [3] = 11 } }
                : (struct Entry){ .tag = 13, .lane.bytes = { [1] = 17, [2] = 19 } };
}

static unsigned checksum(const struct Bundle *bundle) {
    unsigned acc = bundle->end;

    for (int r = 0; r < 2; ++r) {
        for (int c = 0; c < 2; ++c) {
            const struct Entry *entry = &bundle->grid[r][c];
            acc = acc * 61u + entry->tag;
            acc = acc * 59u + entry->lane.bytes[0];
            acc = acc * 53u + entry->lane.bytes[1];
            acc = acc * 47u + entry->lane.bytes[3];
        }
    }

    return acc;
}

int main(void) {
    struct Bundle bundle = {
        .grid[0][0] = select_entry(1),
        .grid[0][1].lane.bytes[2] = 23,
        .grid[1][1] = { .tag = 29, .lane.bytes = { 31, 0, 37, 41 } },
        .end = 43,
    };

    bundle.grid[0][1] = (struct Entry){ .tag = 47, .lane = (union Lane){ .bytes = { [1] = 53, [3] = 59 } } };
    bundle.grid[1][0] = select_entry(0);

    printf("%u %u %u %u %u %u\n",
           (unsigned)offsetof(struct Bundle, end),
           (unsigned)bundle.grid[0][0].lane.bytes[3],
           (unsigned)bundle.grid[0][1].lane.bytes[0],
           (unsigned)bundle.grid[0][1].lane.bytes[3],
           (unsigned)bundle.grid[1][0].lane.bytes[2],
           checksum(&bundle));
    return 0;
}
