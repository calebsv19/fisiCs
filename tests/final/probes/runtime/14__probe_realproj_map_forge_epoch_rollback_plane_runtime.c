#include <stdio.h>

typedef struct {
    unsigned epoch;
    unsigned owner;
    unsigned rollback;
    unsigned plane;
    unsigned resident;
    unsigned guard;
} Tile;

typedef struct {
    char op;
    unsigned lane;
    unsigned arg;
} Event;

int main(void) {
    static const Event events[] = {
        {'E', 0u, 0u}, {'A', 0u, 4u}, {'P', 0u, 2u}, {'R', 0u, 3u},
        {'E', 1u, 0u}, {'A', 1u, 5u}, {'G', 1u, 4u}, {'P', 1u, 1u},
        {'E', 2u, 0u}, {'A', 2u, 6u}, {'R', 2u, 2u}, {'G', 2u, 3u},
        {'P', 2u, 4u}, {'A', 1u, 2u}, {'R', 1u, 1u}, {'G', 0u, 5u},
        {'P', 0u, 3u}, {'A', 2u, 1u},
    };
    Tile tiles[3] = {
        {1u, 3u, 0u, 2u, 2u, 5u},
        {2u, 5u, 1u, 3u, 3u, 7u},
        {3u, 7u, 2u, 4u, 4u, 11u},
    };
    unsigned digest = 2166136261u;
    unsigned i;

    for (i = 0u; i < (unsigned)(sizeof(events) / sizeof(events[0])); ++i) {
        Tile* t = &tiles[events[i].lane % 3u];
        if (events[i].op == 'E') {
            t->epoch += 1u;
        } else if (events[i].op == 'A') {
            t->resident += events[i].arg + (t->owner & 1u);
        } else if (events[i].op == 'P') {
            t->plane = (t->plane + events[i].arg + t->epoch) % 19u;
        } else if (events[i].op == 'R') {
            t->rollback = (t->rollback + events[i].arg + t->plane) % 17u;
        } else if (events[i].op == 'G') {
            t->guard ^= events[i].arg + t->rollback + t->resident;
            t->owner = (t->owner + t->guard + t->plane) % 29u;
        }

        digest ^= t->epoch * 17u;
        digest ^= t->owner * 23u;
        digest ^= t->rollback * 29u;
        digest ^= t->plane * 31u;
        digest ^= t->resident * 37u;
        digest ^= t->guard * 41u;
        digest *= 16777619u;
    }

    printf("%u %u %u\n",
           tiles[0].owner ^ tiles[1].owner ^ tiles[2].owner,
           tiles[0].resident + tiles[1].resident + tiles[2].resident,
           digest);
    return 0;
}
