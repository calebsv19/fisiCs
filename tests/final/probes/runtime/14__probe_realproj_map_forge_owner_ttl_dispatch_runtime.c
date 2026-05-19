#include <stdio.h>

typedef struct {
    unsigned id;
    unsigned generation;
    unsigned owner;
    unsigned ttl;
    unsigned score;
} TileRow;

typedef unsigned (*RowStep)(TileRow* row, unsigned arg, unsigned tick);

static unsigned row_touch(TileRow* row, unsigned arg, unsigned tick) {
    row->generation += 1u;
    row->ttl = (row->ttl + arg + tick) % 7u;
    row->score += row->id * 3u + arg + tick;
    return row->score ^ (row->generation * 19u);
}

static unsigned row_transfer(TileRow* row, unsigned arg, unsigned tick) {
    row->owner = (row->owner + (arg & 3u) + 1u) & 3u;
    row->ttl = (row->ttl + row->owner + tick) % 9u;
    row->score ^= (arg * 13u + row->owner * 17u);
    return row->score + row->ttl * 23u;
}

static unsigned row_trim(TileRow* row, unsigned arg, unsigned tick) {
    unsigned cut = (arg + tick) % 11u;
    if (row->score > cut) {
        row->score -= cut;
    } else {
        row->score = 0u;
    }
    if (row->generation > 0u) {
        row->generation -= 1u;
    }
    return row->score ^ (row->owner * 29u);
}

int main(void) {
    TileRow rows[4] = {
        {41u, 1u, 0u, 1u, 7u},
        {77u, 2u, 1u, 2u, 11u},
        {95u, 1u, 2u, 0u, 5u},
        {109u, 0u, 3u, 3u, 9u},
    };
    RowStep steps[3] = {row_touch, row_transfer, row_trim};
    unsigned digest = 53u;
    unsigned i;

    for (i = 0u; i < 16u; ++i) {
        TileRow* row = &rows[(i * 5u + 2u) % 4u];
        unsigned k = (row->generation + row->owner + row->ttl + i) % 3u;
        unsigned lane = steps[k](row, i * 9u + row->id, i);
        digest = digest * 191u + lane;
    }

    printf("%u %u %u\n",
           rows[0].generation + rows[1].generation + rows[2].generation + rows[3].generation,
           rows[0].owner ^ rows[1].owner ^ rows[2].owner ^ rows[3].owner,
           digest);
    return 0;
}
