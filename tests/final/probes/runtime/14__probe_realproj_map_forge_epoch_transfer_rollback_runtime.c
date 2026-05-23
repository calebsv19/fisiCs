#include <stdio.h>

typedef struct {
    unsigned epoch;
    unsigned owner;
    unsigned credits;
    unsigned replay;
    unsigned handoff;
    unsigned stale;
} Lane;

typedef struct {
    char op;
    unsigned lane;
    unsigned arg;
} Event;

int main(void) {
    static const Event events[] = {
        {'E', 0u, 0u}, {'P', 0u, 4u}, {'H', 0u, 2u}, {'X', 0u, 1u},
        {'E', 1u, 0u}, {'P', 1u, 5u}, {'S', 1u, 3u}, {'X', 1u, 2u},
        {'E', 2u, 0u}, {'P', 2u, 6u}, {'H', 2u, 1u}, {'R', 2u, 4u},
        {'X', 2u, 0u}, {'R', 0u, 3u}, {'S', 0u, 2u}, {'H', 1u, 4u},
        {'P', 1u, 2u}, {'R', 1u, 1u},
    };
    Lane lanes[3] = {
        {1u, 3u, 7u, 0u, 1u, 0u},
        {2u, 5u, 6u, 1u, 2u, 0u},
        {3u, 7u, 5u, 2u, 3u, 0u},
    };
    unsigned digest = 2166136261u;
    unsigned i;

    for (i = 0u; i < (unsigned)(sizeof(events) / sizeof(events[0])); ++i) {
        Lane* l = &lanes[events[i].lane % 3u];
        Lane* peer = &lanes[(events[i].lane + 1u) % 3u];
        if (events[i].op == 'E') {
            l->epoch += 1u;
            l->credits += l->owner & 1u;
        } else if (events[i].op == 'P') {
            l->replay = (l->replay + events[i].arg + l->epoch) % 17u;
        } else if (events[i].op == 'H') {
            l->handoff = (l->handoff + events[i].arg + peer->owner) % 19u;
        } else if (events[i].op == 'X') {
            unsigned moved = (l->credits + l->handoff + events[i].arg) % 4u;
            if (l->credits >= moved) {
                l->credits -= moved;
                peer->credits += moved;
            }
        } else if (events[i].op == 'R') {
            l->stale ^= events[i].arg + l->replay + l->handoff;
            l->epoch += 1u;
        } else if (events[i].op == 'S') {
            l->owner = (l->owner + events[i].arg + l->stale) % 23u;
        }

        digest ^= l->epoch * 17u;
        digest ^= l->owner * 23u;
        digest ^= l->credits * 29u;
        digest ^= l->replay * 31u;
        digest ^= l->handoff * 37u;
        digest ^= l->stale * 41u;
        digest *= 16777619u;
    }

    printf("%u %u %u\n",
           lanes[0].credits ^ lanes[1].credits ^ lanes[2].credits,
           lanes[0].owner + lanes[1].owner + lanes[2].owner,
           digest);
    return 0;
}
