#include <stdio.h>

typedef struct Axis5W5State {
    unsigned int committed[5];
    unsigned int scratch[5];
    unsigned int salt;
    unsigned int token;
    unsigned int in_branch;
} Axis5W5State;

typedef struct Axis5W5Frontier {
    unsigned int committed[5];
    unsigned int salt;
    unsigned int token;
} Axis5W5Frontier;

void axis5_w5_state_init(Axis5W5State* s, unsigned int salt);
void axis5_w5_state_apply_committed(Axis5W5State* s, unsigned int lane, unsigned int value);
void axis5_w5_state_begin_branch(Axis5W5State* s);
void axis5_w5_state_apply_branch(Axis5W5State* s, unsigned int lane, unsigned int value);
void axis5_w5_state_end_branch(Axis5W5State* s, unsigned int commit);
void axis5_w5_state_export_frontier(const Axis5W5State* s, Axis5W5Frontier* out);
void axis5_w5_state_import_frontier(Axis5W5State* s, const Axis5W5Frontier* in);
unsigned int axis5_w5_state_signature(const Axis5W5State* s);

static void axis5_w5_apply_segment(
    Axis5W5State* s,
    const unsigned int* lanes,
    const unsigned int* values,
    int n
) {
    for (int i = 0; i < n; ++i) axis5_w5_state_apply_committed(s, lanes[i], values[i]);
}

static void axis5_w5_apply_branch_segment(
    Axis5W5State* s,
    const unsigned int* lanes,
    const unsigned int* values,
    int n,
    unsigned int commit
) {
    axis5_w5_state_begin_branch(s);
    for (int i = 0; i < n; ++i) axis5_w5_state_apply_branch(s, lanes[i], values[i]);
    axis5_w5_state_end_branch(s, commit);
}

int main(void) {
    const unsigned int prefix_lanes[] = {0u, 2u, 4u, 1u};
    const unsigned int prefix_vals[] = {7u, 5u, 9u, 3u};
    const unsigned int rb_lanes[] = {3u, 1u, 4u};
    const unsigned int rb_vals[] = {11u, 6u, 8u};
    const unsigned int cm_lanes[] = {2u, 0u, 3u};
    const unsigned int cm_vals[] = {4u, 10u, 12u};
    const unsigned int tail_lanes[] = {1u, 4u, 2u};
    const unsigned int tail_vals[] = {13u, 2u, 14u};

    Axis5W5State full;
    Axis5W5State replay;
    Axis5W5Frontier frontier;

    axis5_w5_state_init(&full, 0x4242u);
    axis5_w5_apply_segment(&full, prefix_lanes, prefix_vals, 4);
    axis5_w5_apply_branch_segment(&full, rb_lanes, rb_vals, 3, 0u);
    axis5_w5_apply_branch_segment(&full, cm_lanes, cm_vals, 3, 1u);
    axis5_w5_apply_segment(&full, tail_lanes, tail_vals, 3);

    axis5_w5_state_init(&replay, 0x4242u);
    axis5_w5_apply_segment(&replay, prefix_lanes, prefix_vals, 4);
    axis5_w5_apply_branch_segment(&replay, rb_lanes, rb_vals, 3, 0u);
    axis5_w5_state_export_frontier(&replay, &frontier);

    axis5_w5_state_init(&replay, 0u);
    axis5_w5_state_import_frontier(&replay, &frontier);
    axis5_w5_apply_branch_segment(&replay, cm_lanes, cm_vals, 3, 1u);
    axis5_w5_apply_segment(&replay, tail_lanes, tail_vals, 3);

    const unsigned int sig_full = axis5_w5_state_signature(&full);
    const unsigned int sig_replay = axis5_w5_state_signature(&replay);
    const unsigned int same = (sig_full == sig_replay) ? 1u : 0u;

    printf("%u %u %u\n", sig_full, sig_replay, same);
    return 0;
}
