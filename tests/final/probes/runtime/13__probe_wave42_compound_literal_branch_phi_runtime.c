#include <stdio.h>

typedef union {
    struct {
        int cell[2];
        int bias;
    } grid;
    struct {
        int lo;
        int hi;
        int mark;
    } span;
} Wave42Payload;

typedef struct {
    int tag;
    Wave42Payload payload;
    int tail[2];
} Wave42Packet;

static Wave42Packet make_grid(int seed) {
    return (Wave42Packet){
        1,
        {.grid = {{seed + 1, seed + 4}, seed - 2}},
        {seed + 20, seed + 30}
    };
}

static Wave42Packet make_span(int seed) {
    return (Wave42Packet){
        2,
        {.span = {seed - 3, seed + 7, seed * 5}},
        {seed + 40, seed + 50}
    };
}

static int packet_score(Wave42Packet p) {
    if (p.tag == 1) {
        return p.payload.grid.cell[0] * 3 + p.payload.grid.cell[1] * 5 +
               p.payload.grid.bias + p.tail[0] - p.tail[1];
    }
    return p.payload.span.lo * 7 + p.payload.span.hi * 2 +
           p.payload.span.mark - p.tail[0] + p.tail[1];
}

int main(void) {
    Wave42Packet kept = make_grid(3);
    int total = packet_score(kept);
    int i;

    for (i = 0; i < 6; ++i) {
        Wave42Packet a = make_grid(i + 5);
        Wave42Packet b = make_span(i + 8);
        Wave42Packet selected = ((packet_score(kept) + i) & 1)
            ? a
            : (Wave42Packet){1, {.grid = {{i + 10, i + 12}, i - 1}}, {i + 60, i + 70}};

        if ((i % 3) == 2) {
            selected = b;
        }

        if (packet_score(selected) > packet_score(kept) - i) {
            kept = selected;
        } else {
            kept.tail[i & 1] += i + kept.tag;
        }
        total += packet_score(kept);
    }

    printf("%d %d %d %d\n", kept.tag, kept.tail[0], kept.tail[1], total);
    return 0;
}
