#include <stdio.h>

typedef struct {
    unsigned owner;
    unsigned epoch;
    unsigned resident;
    unsigned borrow;
    unsigned budget;
    unsigned guard;
} Tile;

typedef struct {
    char op;
    unsigned lane;
    unsigned arg;
} Event;

int main(void) {
    static const Event events[] = {
        {'A', 0u, 3u}, {'A', 1u, 5u}, {'B', 0u, 2u}, {'E', 1u, 0u},
        {'C', 0u, 0u}, {'H', 1u, 4u}, {'A', 2u, 6u}, {'B', 2u, 1u},
        {'C', 2u, 0u}, {'T', 1u, 2u}, {'B', 1u, 3u}, {'E', 0u, 0u},
        {'A', 0u, 4u}, {'H', 2u, 1u}, {'C', 1u, 0u}, {'T', 0u, 3u},
    };
    Tile tiles[3] = {
        {3u, 1u, 2u, 0u, 8u, 5u},
        {5u, 2u, 3u, 1u, 7u, 7u},
        {7u, 3u, 4u, 2u, 6u, 11u},
    };
    unsigned digest = 2166136261u;
    unsigned commits = 0u;
    unsigned trims = 0u;
    unsigned i;

    for (i = 0u; i < (unsigned)(sizeof(events) / sizeof(events[0])); ++i) {
        Tile* t = &tiles[events[i].lane % 3u];
        if (events[i].op == 'A') {
            t->resident += events[i].arg + (t->owner & 1u);
        } else if (events[i].op == 'B') {
            t->borrow = (t->borrow + events[i].arg + t->epoch) % 13u;
        } else if (events[i].op == 'E') {
            t->epoch += 1u;
            t->guard ^= t->borrow + t->resident;
        } else if (events[i].op == 'C') {
            t->owner = (t->owner + t->borrow + t->epoch) % 23u;
            commits += 1u;
        } else if (events[i].op == 'H') {
            t->budget = (t->budget + events[i].arg + t->guard) % 17u + 4u;
        } else if (events[i].op == 'T') {
            while (t->resident > t->budget) {
                t->resident -= 1u;
                trims += 1u;
            }
        }

        digest ^= t->owner * 17u;
        digest ^= t->epoch * 23u;
        digest ^= t->resident * 29u;
        digest ^= t->borrow * 31u;
        digest ^= t->budget * 37u;
        digest ^= t->guard * 41u;
        digest *= 16777619u;
    }

    printf("%u %u %u\n", commits, trims, digest);
    return 0;
}
