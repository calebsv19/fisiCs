#include <stdio.h>

typedef struct Axis5W4State {
    unsigned int bins[5];
    unsigned int epoch;
    unsigned int salt;
} Axis5W4State;

typedef struct Axis5W4Frontier {
    unsigned int bins[5];
    unsigned int epoch;
    unsigned int token;
} Axis5W4Frontier;

void axis5_w4_state_init(Axis5W4State* s, unsigned int salt);
void axis5_w4_state_apply_batch(
    Axis5W4State* s,
    const unsigned int* lanes,
    const unsigned int* values,
    const unsigned int* fences,
    int n
);
void axis5_w4_state_export_frontier(const Axis5W4State* s, Axis5W4Frontier* out);
void axis5_w4_state_import_frontier(Axis5W4State* s, const Axis5W4Frontier* in);
unsigned int axis5_w4_state_signature(const Axis5W4State* s);

int main(void) {
    const unsigned int lanes[] = {0u, 3u, 1u, 4u, 2u, 0u, 1u, 3u, 4u, 2u};
    const unsigned int vals[] = {6u, 2u, 7u, 5u, 9u, 3u, 8u, 4u, 10u, 11u};
    const unsigned int fences[] = {0u, 0u, 0u, 1u, 0u, 0u, 1u, 0u, 0u, 0u};

    Axis5W4State full;
    Axis5W4State replay;
    Axis5W4Frontier frontier;

    axis5_w4_state_init(&full, 0x33a5u);
    axis5_w4_state_apply_batch(&full, lanes, vals, fences, 10);

    axis5_w4_state_init(&replay, 0x33a5u);
    axis5_w4_state_apply_batch(&replay, lanes, vals, fences, 6);
    axis5_w4_state_export_frontier(&replay, &frontier);

    axis5_w4_state_init(&replay, 0u);
    axis5_w4_state_import_frontier(&replay, &frontier);
    axis5_w4_state_apply_batch(&replay, lanes + 6, vals + 6, fences + 6, 4);

    const unsigned int sig_full = axis5_w4_state_signature(&full);
    const unsigned int sig_replay = axis5_w4_state_signature(&replay);
    const unsigned int same = (sig_full == sig_replay) ? 1u : 0u;

    printf("%u %u %u\n", sig_full, sig_replay, same);
    return 0;
}
