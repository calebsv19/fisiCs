#include "15__probe_wave138_realproj_tile_dispatch_shared.h"

unsigned wave138_step_reclaim(Wave138TileRow* row, unsigned arg, unsigned tick) {
    unsigned cut = (arg + tick + row->owner) % 19u;

    if (row->score > cut) {
        row->score -= cut;
    } else {
        row->score = 0u;
    }
    if (row->generation > 0u) {
        row->generation -= 1u;
    }
    row->guard ^= (cut * 31u + row->generation);
    return row->score ^ (row->guard * 5u);
}

unsigned wave138_tile_dispatch_checksum(Wave138TileRow rows[4],
                                        Wave138StepFn steps[3]) {
    unsigned checksum = 89u;
    unsigned i;

    for (i = 0u; i < 20u; ++i) {
        Wave138TileRow* row = &rows[(i * 9u + 1u) % 4u];
        unsigned step_index =
            (row->generation + row->owner + row->ttl + row->guard + i) % 3u;
        unsigned lane = steps[step_index](row, i * 13u + row->id, i);
        checksum = checksum * 199u + lane;
    }

    return checksum;
}
