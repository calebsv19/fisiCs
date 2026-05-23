#include <stdio.h>

typedef struct {
    unsigned owner;
    unsigned budget;
    unsigned resident;
    unsigned rollback;
    unsigned guard;
    unsigned epoch;
} Tile;

typedef struct {
    char op;
    unsigned lane;
    unsigned arg;
} Event;

int main(void) {
    static const Event events[] = {
        {'A', 0u, 5u}, {'A', 1u, 3u}, {'R', 0u, 2u}, {'G', 1u, 4u},
        {'C', 0u, 0u}, {'T', 1u, 2u}, {'A', 2u, 6u}, {'R', 2u, 1u},
        {'T', 0u, 1u}, {'B', 2u, 7u}, {'C', 2u, 0u}, {'G', 0u, 3u},
        {'R', 1u, 4u}, {'B', 1u, 5u}, {'C', 1u, 0u}, {'T', 2u, 3u},
    };
    Tile tiles[3] = {
        {3u, 8u, 2u, 0u, 5u, 1u},
        {5u, 7u, 3u, 0u, 7u, 2u},
        {7u, 6u, 4u, 0u, 11u, 3u},
    };
    unsigned digest = 2166136261u;
    unsigned commits = 0u;
    unsigned trims = 0u;
    unsigned i;

    for (i = 0u; i < (unsigned)(sizeof(events) / sizeof(events[0])); ++i) {
        Tile* t = &tiles[events[i].lane % 3u];
        if (events[i].op == 'A') {
            t->resident += events[i].arg + (t->owner & 1u);
            t->epoch += 1u;
        } else if (events[i].op == 'R') {
            t->rollback = (t->rollback + events[i].arg + t->guard) % 13u;
            if (t->resident > (events[i].arg & 1u)) {
                t->resident -= (events[i].arg & 1u);
            }
        } else if (events[i].op == 'G') {
            t->guard ^= events[i].arg + t->epoch + t->rollback;
        } else if (events[i].op == 'C') {
            t->owner = (t->owner + t->rollback + t->epoch) % 17u;
            commits += 1u;
        } else if (events[i].op == 'T') {
            t->owner = (t->owner + events[i].arg + t->guard) % 19u;
        } else if (events[i].op == 'B') {
            t->budget = events[i].arg;
            while (t->resident > t->budget) {
                t->resident -= 1u;
                trims += 1u;
            }
        }

        digest ^= t->owner * 17u;
        digest ^= t->budget * 23u;
        digest ^= t->resident * 29u;
        digest ^= t->rollback * 31u;
        digest ^= t->guard * 37u;
        digest ^= t->epoch * 41u;
        digest *= 16777619u;
    }

    printf("%u %u %u\n", commits, trims, digest);
    return 0;
}
