#include <stdio.h>

typedef struct {
    unsigned owner;
    unsigned borrow;
    unsigned trim;
    unsigned rollback;
    unsigned resident;
    unsigned budget;
} Lane;

typedef struct {
    char op;
    unsigned lane;
    unsigned arg;
} Event;

int main(void) {
    static const Event events[] = {
        {'B', 0u, 2u}, {'A', 0u, 4u}, {'T', 0u, 0u}, {'R', 0u, 1u},
        {'B', 1u, 3u}, {'A', 1u, 5u}, {'R', 1u, 2u}, {'T', 1u, 0u},
        {'B', 2u, 1u}, {'A', 2u, 6u}, {'T', 2u, 0u}, {'R', 2u, 3u},
        {'A', 1u, 2u}, {'B', 0u, 4u}, {'T', 0u, 0u}, {'R', 1u, 1u},
        {'A', 2u, 3u}, {'T', 2u, 0u},
    };
    Lane lanes[3] = {
        {3u, 0u, 1u, 0u, 2u, 8u},
        {5u, 1u, 2u, 1u, 3u, 7u},
        {7u, 2u, 3u, 2u, 4u, 6u},
    };
    unsigned digest = 2166136261u;
    unsigned trims = 0u;
    unsigned i;

    for (i = 0u; i < (unsigned)(sizeof(events) / sizeof(events[0])); ++i) {
        Lane* l = &lanes[events[i].lane % 3u];
        if (events[i].op == 'B') {
            l->borrow = (l->borrow + events[i].arg + l->owner) % 19u;
        } else if (events[i].op == 'A') {
            l->resident += events[i].arg + (l->borrow & 1u);
        } else if (events[i].op == 'T') {
            while (l->resident > l->budget) {
                l->resident -= 1u;
                trims += 1u;
            }
            l->trim ^= l->resident + l->borrow;
        } else if (events[i].op == 'R') {
            l->rollback = (l->rollback + events[i].arg + l->trim) % 23u;
            l->owner = (l->owner + l->rollback + l->borrow) % 31u;
        }

        digest ^= l->owner * 17u;
        digest ^= l->borrow * 23u;
        digest ^= l->trim * 29u;
        digest ^= l->rollback * 31u;
        digest ^= l->resident * 37u;
        digest ^= l->budget * 41u;
        digest *= 16777619u;
    }

    printf("%u %u %u\n",
           lanes[0].owner + lanes[1].owner + lanes[2].owner,
           trims,
           digest);
    return 0;
}
