#include "15__probe_wave138_realproj_tile_dispatch_shared.h"

unsigned wave138_step_update(Wave138TileRow* row, unsigned arg, unsigned tick) {
    row->generation += 1u + (arg & 1u);
    row->ttl = (row->ttl + arg + tick) % 15u;
    row->score += row->id * 7u + arg + tick;
    row->guard ^= (row->generation * 17u) + (row->ttl * 9u);
    return row->score ^ row->guard;
}

unsigned wave138_step_owner(Wave138TileRow* row, unsigned arg, unsigned tick) {
    row->owner = (row->owner + (arg & 3u) + 1u) & 3u;
    row->ttl = (row->ttl + row->owner + tick) % 17u;
    row->score ^= (arg * 19u + row->owner * 23u);
    row->guard += row->owner * 29u + tick;
    return row->score + row->guard;
}
