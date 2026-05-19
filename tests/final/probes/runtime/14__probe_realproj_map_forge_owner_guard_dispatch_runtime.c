#include <stdio.h>

typedef struct {
    unsigned id;
    unsigned generation;
    unsigned owner;
    unsigned ttl;
    unsigned score;
    unsigned guard;
} TileRow;

typedef unsigned (*StepFn)(TileRow* row, unsigned arg, unsigned tick);

static unsigned step_update(TileRow* row, unsigned arg, unsigned tick) {
    row->generation += 1u + (arg & 1u);
    row->ttl = (row->ttl + arg + tick) % 11u;
    row->score += row->id * 5u + arg + tick;
    row->guard ^= (row->generation * 13u) + (row->ttl * 7u);
    return row->score ^ row->guard;
}

static unsigned step_owner(TileRow* row, unsigned arg, unsigned tick) {
    row->owner = (row->owner + (arg & 3u) + 1u) & 3u;
    row->ttl = (row->ttl + row->owner + tick) % 13u;
    row->score ^= (arg * 17u + row->owner * 19u);
    row->guard += row->owner * 23u + tick;
    return row->score + row->guard;
}

static unsigned step_reclaim(TileRow* row, unsigned arg, unsigned tick) {
    unsigned cut = (arg + tick + row->owner) % 17u;
    if (row->score > cut) {
        row->score -= cut;
    } else {
        row->score = 0u;
    }
    if (row->generation > 0u) {
        row->generation -= 1u;
    }
    row->guard ^= (cut * 29u + row->generation);
    return row->score ^ (row->guard * 3u);
}

int main(void) {
    TileRow rows[4] = {
        {41u, 1u, 0u, 1u, 7u, 3u},
        {77u, 2u, 1u, 2u, 11u, 5u},
        {95u, 1u, 2u, 0u, 5u, 7u},
        {109u, 0u, 3u, 3u, 9u, 11u},
    };
    StepFn steps[3] = {step_update, step_owner, step_reclaim};
    unsigned digest = 71u;
    unsigned i;

    for (i = 0u; i < 18u; ++i) {
        TileRow* row = &rows[(i * 7u + 1u) % 4u];
        unsigned k = (row->generation + row->owner + row->ttl + row->guard + i) % 3u;
        unsigned lane = steps[k](row, i * 11u + row->id, i);
        digest = digest * 193u + lane;
    }

    printf("%u %u %u %u\n",
           rows[0].generation + rows[1].generation + rows[2].generation + rows[3].generation,
           rows[0].owner ^ rows[1].owner ^ rows[2].owner ^ rows[3].owner,
           rows[0].guard + rows[1].guard + rows[2].guard + rows[3].guard,
           digest);
    return 0;
}
