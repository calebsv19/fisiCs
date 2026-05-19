#include <stdio.h>

typedef struct Axis5W14Snapshot {
    unsigned int shard;
    unsigned int checkpoint;
    unsigned int frontier;
    unsigned int epoch;
    unsigned int lanes[2];
} Axis5W14Snapshot;

typedef struct Axis5W14Aggregate {
    unsigned int checkpoint_by_shard[3];
    unsigned int frontier_by_shard[3];
    unsigned int epoch_by_shard[3];
    unsigned int lanes_by_shard[3][2];
} Axis5W14Aggregate;

void axis5_w14_seed_snapshot(
    Axis5W14Snapshot* out,
    unsigned int shard,
    unsigned int checkpoint,
    unsigned int frontier,
    unsigned int epoch,
    unsigned int a,
    unsigned int b
);
void axis5_w14_encode_snapshot(const Axis5W14Snapshot* s, unsigned int wire[6]);
void axis5_w14_decode_snapshot(Axis5W14Snapshot* s, const unsigned int wire[6]);
void axis5_w14_clear_aggregate(Axis5W14Aggregate* a);
void axis5_w14_absorb_snapshot(Axis5W14Aggregate* a, const Axis5W14Snapshot* s);
unsigned int axis5_w14_snapshot_signature(const Axis5W14Aggregate* a);

int main(void) {
    Axis5W14Snapshot s0_old;
    Axis5W14Snapshot s0_new;
    Axis5W14Snapshot s1_old;
    Axis5W14Snapshot s1_new;
    Axis5W14Snapshot s2_old;
    Axis5W14Snapshot s2_new;
    Axis5W14Snapshot decoded;
    Axis5W14Aggregate canonical;
    Axis5W14Aggregate replayed;
    unsigned int w0_old[6];
    unsigned int w0_new[6];
    unsigned int w1_old[6];
    unsigned int w1_new[6];
    unsigned int w2_old[6];
    unsigned int w2_new[6];

    axis5_w14_seed_snapshot(&s0_old, 0u, 2u, 3u, 1u, 4u, 7u);
    axis5_w14_seed_snapshot(&s0_new, 0u, 6u, 5u, 3u, 9u, 12u);
    axis5_w14_seed_snapshot(&s1_old, 1u, 4u, 2u, 5u, 8u, 3u);
    axis5_w14_seed_snapshot(&s1_new, 1u, 7u, 4u, 6u, 14u, 6u);
    axis5_w14_seed_snapshot(&s2_old, 2u, 3u, 1u, 2u, 5u, 9u);
    axis5_w14_seed_snapshot(&s2_new, 2u, 8u, 6u, 4u, 10u, 15u);

    axis5_w14_encode_snapshot(&s0_old, w0_old);
    axis5_w14_encode_snapshot(&s0_new, w0_new);
    axis5_w14_encode_snapshot(&s1_old, w1_old);
    axis5_w14_encode_snapshot(&s1_new, w1_new);
    axis5_w14_encode_snapshot(&s2_old, w2_old);
    axis5_w14_encode_snapshot(&s2_new, w2_new);

    axis5_w14_clear_aggregate(&canonical);
    axis5_w14_absorb_snapshot(&canonical, &s0_new);
    axis5_w14_absorb_snapshot(&canonical, &s1_new);
    axis5_w14_absorb_snapshot(&canonical, &s2_new);

    axis5_w14_clear_aggregate(&replayed);
    axis5_w14_decode_snapshot(&decoded, w2_old);
    axis5_w14_absorb_snapshot(&replayed, &decoded);
    axis5_w14_decode_snapshot(&decoded, w0_new);
    axis5_w14_absorb_snapshot(&replayed, &decoded);
    axis5_w14_decode_snapshot(&decoded, w1_old);
    axis5_w14_absorb_snapshot(&replayed, &decoded);
    axis5_w14_decode_snapshot(&decoded, w2_new);
    axis5_w14_absorb_snapshot(&replayed, &decoded);
    axis5_w14_decode_snapshot(&decoded, w0_old);
    axis5_w14_absorb_snapshot(&replayed, &decoded);
    axis5_w14_decode_snapshot(&decoded, w1_new);
    axis5_w14_absorb_snapshot(&replayed, &decoded);
    axis5_w14_decode_snapshot(&decoded, w2_new);
    axis5_w14_absorb_snapshot(&replayed, &decoded);

    {
        unsigned int sig_canonical = axis5_w14_snapshot_signature(&canonical);
        unsigned int sig_replayed = axis5_w14_snapshot_signature(&replayed);
        unsigned int same = (sig_canonical == sig_replayed) ? 1u : 0u;
        printf("%u %u %u\n", sig_canonical, sig_replayed, same);
    }
    return 0;
}
