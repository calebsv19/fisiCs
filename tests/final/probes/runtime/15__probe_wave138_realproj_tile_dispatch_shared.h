#ifndef FISICS_PROBE_WAVE138_REALPROJ_TILE_DISPATCH_SHARED_H
#define FISICS_PROBE_WAVE138_REALPROJ_TILE_DISPATCH_SHARED_H

/* Multi-TU extraction of the callback/state seam in
 * 15__probe_corpus_realproj_map_forge_app_fragment_m_smoke.c. */
typedef struct {
    unsigned id;
    unsigned generation;
    unsigned owner;
    unsigned ttl;
    unsigned score;
    unsigned guard;
} Wave138TileRow;

typedef unsigned (*Wave138StepFn)(Wave138TileRow* row,
                                  unsigned arg,
                                  unsigned tick);

unsigned wave138_step_update(Wave138TileRow* row, unsigned arg, unsigned tick);
unsigned wave138_step_owner(Wave138TileRow* row, unsigned arg, unsigned tick);
unsigned wave138_step_reclaim(Wave138TileRow* row, unsigned arg, unsigned tick);
unsigned wave138_tile_dispatch_checksum(Wave138TileRow rows[4],
                                        Wave138StepFn steps[3]);

#endif
