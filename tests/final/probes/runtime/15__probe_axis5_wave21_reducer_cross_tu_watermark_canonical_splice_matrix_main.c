#include <stdio.h>

typedef struct Axis5W21Snapshot {
    unsigned int shard;
    unsigned int checkpoint;
    unsigned int watermark;
    unsigned int lane_a;
    unsigned int lane_b;
} Axis5W21Snapshot;

typedef struct Axis5W21Aggregate {
    unsigned int checkpoint[4];
    unsigned int watermark[4];
    unsigned int lane_a[4];
    unsigned int lane_b[4];
} Axis5W21Aggregate;

void axis5_w21_seed_snapshot(
    Axis5W21Snapshot* out,
    unsigned int shard,
    unsigned int checkpoint,
    unsigned int watermark,
    unsigned int lane_a,
    unsigned int lane_b
);
void axis5_w21_encode_snapshot(const Axis5W21Snapshot* s, unsigned int wire[5]);
void axis5_w21_decode_snapshot(Axis5W21Snapshot* s, const unsigned int wire[5]);
void axis5_w21_clear_aggregate(Axis5W21Aggregate* a);
void axis5_w21_absorb_snapshot(Axis5W21Aggregate* a, const Axis5W21Snapshot* s);
unsigned int axis5_w21_snapshot_signature(const Axis5W21Aggregate* a);

int main(void) {
    Axis5W21Snapshot s0_old;
    Axis5W21Snapshot s0_new;
    Axis5W21Snapshot s1_old;
    Axis5W21Snapshot s1_new;
    Axis5W21Snapshot s2_old;
    Axis5W21Snapshot s2_new;
    Axis5W21Snapshot s3_old;
    Axis5W21Snapshot s3_new;
    Axis5W21Snapshot decoded;
    Axis5W21Aggregate canonical;
    Axis5W21Aggregate replayed;
    unsigned int w0_old[5];
    unsigned int w0_new[5];
    unsigned int w1_old[5];
    unsigned int w1_new[5];
    unsigned int w2_old[5];
    unsigned int w2_new[5];
    unsigned int w3_old[5];
    unsigned int w3_new[5];

    axis5_w21_seed_snapshot(&s0_old, 0u, 3u, 2u, 5u, 6u);
    axis5_w21_seed_snapshot(&s0_new, 0u, 6u, 5u, 9u, 11u);
    axis5_w21_seed_snapshot(&s1_old, 1u, 2u, 1u, 4u, 7u);
    axis5_w21_seed_snapshot(&s1_new, 1u, 7u, 6u, 10u, 12u);
    axis5_w21_seed_snapshot(&s2_old, 2u, 4u, 3u, 8u, 5u);
    axis5_w21_seed_snapshot(&s2_new, 2u, 8u, 7u, 14u, 15u);
    axis5_w21_seed_snapshot(&s3_old, 3u, 5u, 2u, 3u, 9u);
    axis5_w21_seed_snapshot(&s3_new, 3u, 9u, 8u, 16u, 13u);

    axis5_w21_encode_snapshot(&s0_old, w0_old);
    axis5_w21_encode_snapshot(&s0_new, w0_new);
    axis5_w21_encode_snapshot(&s1_old, w1_old);
    axis5_w21_encode_snapshot(&s1_new, w1_new);
    axis5_w21_encode_snapshot(&s2_old, w2_old);
    axis5_w21_encode_snapshot(&s2_new, w2_new);
    axis5_w21_encode_snapshot(&s3_old, w3_old);
    axis5_w21_encode_snapshot(&s3_new, w3_new);

    axis5_w21_clear_aggregate(&canonical);
    axis5_w21_absorb_snapshot(&canonical, &s0_new);
    axis5_w21_absorb_snapshot(&canonical, &s1_new);
    axis5_w21_absorb_snapshot(&canonical, &s2_new);
    axis5_w21_absorb_snapshot(&canonical, &s3_new);

    axis5_w21_clear_aggregate(&replayed);
    axis5_w21_decode_snapshot(&decoded, w2_old);
    axis5_w21_absorb_snapshot(&replayed, &decoded);
    axis5_w21_decode_snapshot(&decoded, w1_new);
    axis5_w21_absorb_snapshot(&replayed, &decoded);
    axis5_w21_decode_snapshot(&decoded, w0_old);
    axis5_w21_absorb_snapshot(&replayed, &decoded);
    axis5_w21_decode_snapshot(&decoded, w3_new);
    axis5_w21_absorb_snapshot(&replayed, &decoded);
    axis5_w21_decode_snapshot(&decoded, w2_new);
    axis5_w21_absorb_snapshot(&replayed, &decoded);
    axis5_w21_decode_snapshot(&decoded, w1_old);
    axis5_w21_absorb_snapshot(&replayed, &decoded);
    axis5_w21_decode_snapshot(&decoded, w0_new);
    axis5_w21_absorb_snapshot(&replayed, &decoded);
    axis5_w21_decode_snapshot(&decoded, w3_old);
    axis5_w21_absorb_snapshot(&replayed, &decoded);
    axis5_w21_decode_snapshot(&decoded, w2_old);
    axis5_w21_absorb_snapshot(&replayed, &decoded);
    axis5_w21_decode_snapshot(&decoded, w1_old);
    axis5_w21_absorb_snapshot(&replayed, &decoded);
    axis5_w21_decode_snapshot(&decoded, w0_old);
    axis5_w21_absorb_snapshot(&replayed, &decoded);

    {
        unsigned int sig_canonical = axis5_w21_snapshot_signature(&canonical);
        unsigned int sig_replayed = axis5_w21_snapshot_signature(&replayed);
        unsigned int same = (sig_canonical == sig_replayed) ? 1u : 0u;
        printf("%u %u %u\n", sig_canonical, sig_replayed, same);
    }
    return 0;
}
