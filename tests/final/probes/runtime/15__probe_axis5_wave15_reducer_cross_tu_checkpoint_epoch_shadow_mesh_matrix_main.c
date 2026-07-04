#include <stdio.h>

typedef struct Axis5W15CESSnapshot {
    unsigned int shard;
    unsigned int checkpoint;
    unsigned int epoch;
    unsigned int shadow;
    int lane_sum;
} Axis5W15CESSnapshot;

typedef struct Axis5W15CESAggregate {
    unsigned int checkpoint[3];
    unsigned int epoch[3];
    unsigned int shadow[3];
    int lane_sum[3];
} Axis5W15CESAggregate;

void axis5_w15_ces_seed_snapshot(
    Axis5W15CESSnapshot* out,
    unsigned int shard,
    unsigned int checkpoint,
    unsigned int epoch,
    unsigned int shadow,
    int lane_sum
);
void axis5_w15_ces_encode_snapshot(const Axis5W15CESSnapshot* s, unsigned int wire[5]);
void axis5_w15_ces_decode_snapshot(Axis5W15CESSnapshot* s, const unsigned int wire[5]);
void axis5_w15_ces_clear_aggregate(Axis5W15CESAggregate* a);
void axis5_w15_ces_absorb_snapshot(Axis5W15CESAggregate* a, const Axis5W15CESSnapshot* s);
unsigned int axis5_w15_ces_signature(const Axis5W15CESAggregate* a);

int main(void) {
    Axis5W15CESSnapshot s0_old;
    Axis5W15CESSnapshot s0_new;
    Axis5W15CESSnapshot s1_old;
    Axis5W15CESSnapshot s1_new;
    Axis5W15CESSnapshot s2_old;
    Axis5W15CESSnapshot s2_new;
    Axis5W15CESSnapshot decoded;
    Axis5W15CESAggregate canonical;
    Axis5W15CESAggregate replayed;
    unsigned int w0_old[5];
    unsigned int w0_new[5];
    unsigned int w1_old[5];
    unsigned int w1_new[5];
    unsigned int w2_old[5];
    unsigned int w2_new[5];

    axis5_w15_ces_seed_snapshot(&s0_old, 0u, 3u, 1u, 1u, 8);
    axis5_w15_ces_seed_snapshot(&s0_new, 0u, 6u, 3u, 4u, 17);
    axis5_w15_ces_seed_snapshot(&s1_old, 1u, 2u, 2u, 2u, 6);
    axis5_w15_ces_seed_snapshot(&s1_new, 1u, 7u, 4u, 5u, 19);
    axis5_w15_ces_seed_snapshot(&s2_old, 2u, 4u, 1u, 1u, 9);
    axis5_w15_ces_seed_snapshot(&s2_new, 2u, 8u, 5u, 6u, 23);

    axis5_w15_ces_encode_snapshot(&s0_old, w0_old);
    axis5_w15_ces_encode_snapshot(&s0_new, w0_new);
    axis5_w15_ces_encode_snapshot(&s1_old, w1_old);
    axis5_w15_ces_encode_snapshot(&s1_new, w1_new);
    axis5_w15_ces_encode_snapshot(&s2_old, w2_old);
    axis5_w15_ces_encode_snapshot(&s2_new, w2_new);

    axis5_w15_ces_clear_aggregate(&canonical);
    axis5_w15_ces_absorb_snapshot(&canonical, &s0_new);
    axis5_w15_ces_absorb_snapshot(&canonical, &s1_new);
    axis5_w15_ces_absorb_snapshot(&canonical, &s2_new);

    axis5_w15_ces_clear_aggregate(&replayed);
    axis5_w15_ces_decode_snapshot(&decoded, w1_old);
    axis5_w15_ces_absorb_snapshot(&replayed, &decoded);
    axis5_w15_ces_decode_snapshot(&decoded, w2_new);
    axis5_w15_ces_absorb_snapshot(&replayed, &decoded);
    axis5_w15_ces_decode_snapshot(&decoded, w0_old);
    axis5_w15_ces_absorb_snapshot(&replayed, &decoded);
    axis5_w15_ces_decode_snapshot(&decoded, w1_new);
    axis5_w15_ces_absorb_snapshot(&replayed, &decoded);
    axis5_w15_ces_decode_snapshot(&decoded, w2_old);
    axis5_w15_ces_absorb_snapshot(&replayed, &decoded);
    axis5_w15_ces_decode_snapshot(&decoded, w0_new);
    axis5_w15_ces_absorb_snapshot(&replayed, &decoded);

    {
        unsigned int sig_canonical = axis5_w15_ces_signature(&canonical);
        unsigned int sig_replayed = axis5_w15_ces_signature(&replayed);
        printf("%u %u %u\n", sig_canonical, sig_replayed, (sig_canonical == sig_replayed) ? 1u : 0u);
    }
    return 0;
}
