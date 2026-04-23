#include <stdio.h>

typedef struct Axis5W1Pair {
    unsigned int op;
    int value;
} Axis5W1Pair;

unsigned int axis5_w1_lib_signature(const Axis5W1Pair* seq, int n, unsigned int salt);
unsigned int axis5_w1_lib_partitioned_signature(
    const Axis5W1Pair* left,
    int n_left,
    const Axis5W1Pair* right,
    int n_right,
    unsigned int salt
);

int main(void) {
    const Axis5W1Pair seq[] = {
        {3u, 19}, {2u, 41}, {7u, 11}, {1u, 33}, {4u, 17}, {5u, 29},
    };
    const unsigned int sig_full = axis5_w1_lib_signature(seq, 6, 0x13579bdfu);
    const unsigned int sig_split = axis5_w1_lib_partitioned_signature(seq, 3, seq + 3, 3, 0x13579bdfu);
    const unsigned int stable = (sig_full != 0u && sig_split != 0u) ? 1u : 0u;

    printf("%u %u %u\n", sig_full, sig_split, stable);
    return 0;
}
