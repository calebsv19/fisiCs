#include <stdio.h>

/*
 * Defined-behavior policy for this differential fixture:
 * - Every compound literal has automatic storage and is used only within the
 *   enclosing block.
 * - The two address comparisons test equality only; no pointer ordering or
 *   cross-object dereference is performed.
 * - All arithmetic is unsigned, so modulo-2^N wrap is defined, and every
 *   shift count is in [0, 7].
 * - Every aggregate member is initialized before it is read.
 */
struct cell {
    unsigned value;
    unsigned lane;
};

struct packet {
    unsigned sum;
    unsigned trace;
    struct cell cells[3];
};

static struct packet advance(struct packet input, unsigned salt) {
    struct cell *left = &(struct cell){ .value = input.cells[0].value + salt, .lane = 1u };
    struct cell *right = &(struct cell){ .lane = 2u, .value = input.cells[1].value + (salt << 1u) };
    unsigned distinct = left != right;
    struct packet next = (struct packet){
        .trace = (input.trace << 3u) ^ left->value ^ (right->value << 1u) ^ distinct,
        .cells = {
            [0] = *left,
            [1] = *right,
            [2] = (struct cell){ .value = input.cells[2].value ^ salt, .lane = 3u }
        },
        .sum = input.sum + left->value + right->value + (input.cells[2].value ^ salt)
    };
    return next;
}

int main(void) {
    struct packet state = (struct packet){
        .sum = 7u,
        .trace = 11u,
        .cells = {
            [0] = (struct cell){ .value = 3u, .lane = 0u },
            [1] = (struct cell){ .value = 5u, .lane = 0u },
            [2] = (struct cell){ .value = 9u, .lane = 0u }
        }
    };
    unsigned salt;

    for (salt = 1u; salt <= 5u; ++salt) {
        struct packet copied = advance(state, salt);
        state = copied;
    }

    printf("axis8-compound=%u\n", state.sum * 257u + state.trace);
    return 0;
}
