#include <stdio.h>

typedef struct Axis5W3State {
    unsigned int bins[6];
    unsigned int salt;
} Axis5W3State;

void axis5_w3_state_init(Axis5W3State* s, unsigned int salt);
void axis5_w3_state_apply_batch(
    Axis5W3State* s,
    const unsigned int* lanes,
    const unsigned int* values,
    int n
);
void axis5_w3_state_checkpoint_encode(const Axis5W3State* s, unsigned int wire[7]);
void axis5_w3_state_checkpoint_decode(Axis5W3State* s, const unsigned int wire[7]);
unsigned int axis5_w3_state_signature(const Axis5W3State* s);

int main(void) {
    const unsigned int lanes[] = {0u, 3u, 1u, 5u, 2u, 4u, 0u, 1u, 3u, 5u};
    const unsigned int vals[] = {8u, 2u, 6u, 9u, 4u, 3u, 11u, 7u, 5u, 10u};
    unsigned int wire[7];

    Axis5W3State full;
    Axis5W3State resume;

    axis5_w3_state_init(&full, 0x1357u);
    axis5_w3_state_apply_batch(&full, lanes, vals, 10);

    axis5_w3_state_init(&resume, 0x1357u);
    axis5_w3_state_apply_batch(&resume, lanes, vals, 4);
    axis5_w3_state_checkpoint_encode(&resume, wire);
    axis5_w3_state_init(&resume, 0u);
    axis5_w3_state_checkpoint_decode(&resume, wire);
    axis5_w3_state_apply_batch(&resume, lanes + 4, vals + 4, 6);

    const unsigned int sig_full = axis5_w3_state_signature(&full);
    const unsigned int sig_resume = axis5_w3_state_signature(&resume);
    const unsigned int same = (sig_full == sig_resume) ? 1u : 0u;

    printf("%u %u %u\n", sig_full, sig_resume, same);
    return 0;
}
