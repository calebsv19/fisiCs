#include <stdio.h>

typedef struct Axis5W10Snapshot {
    unsigned int shard;
    unsigned int epoch;
    unsigned int bins[3];
    unsigned int token;
} Axis5W10Snapshot;

typedef struct Axis5W10Aggregate {
    unsigned int epoch_by_shard[2];
    unsigned int bins_by_shard[2][3];
    unsigned int token_mix;
} Axis5W10Aggregate;

void axis5_w10_seed_snapshot(
    Axis5W10Snapshot* out,
    unsigned int shard,
    unsigned int epoch,
    unsigned int a,
    unsigned int b,
    unsigned int c
);
void axis5_w10_encode_snapshot(const Axis5W10Snapshot* s, unsigned int wire[6]);
void axis5_w10_decode_snapshot(Axis5W10Snapshot* s, const unsigned int wire[6]);
void axis5_w10_clear_aggregate(Axis5W10Aggregate* a);
void axis5_w10_absorb_snapshot(Axis5W10Aggregate* a, const Axis5W10Snapshot* s);
unsigned int axis5_w10_aggregate_signature(const Axis5W10Aggregate* a);

int main(void) {
    Axis5W10Snapshot shard0_epoch2;
    Axis5W10Snapshot shard1_epoch1;
    Axis5W10Snapshot shard1_epoch3;
    Axis5W10Snapshot decoded;
    Axis5W10Aggregate direct;
    Axis5W10Aggregate replayed;
    unsigned int w0[6];
    unsigned int w1[6];
    unsigned int w2[6];

    axis5_w10_seed_snapshot(&shard0_epoch2, 0u, 2u, 5u, 7u, 9u);
    axis5_w10_seed_snapshot(&shard1_epoch1, 1u, 1u, 3u, 4u, 8u);
    axis5_w10_seed_snapshot(&shard1_epoch3, 1u, 3u, 6u, 10u, 12u);

    axis5_w10_encode_snapshot(&shard0_epoch2, w0);
    axis5_w10_encode_snapshot(&shard1_epoch1, w1);
    axis5_w10_encode_snapshot(&shard1_epoch3, w2);

    axis5_w10_clear_aggregate(&direct);
    axis5_w10_absorb_snapshot(&direct, &shard0_epoch2);
    axis5_w10_absorb_snapshot(&direct, &shard1_epoch3);

    axis5_w10_clear_aggregate(&replayed);

    axis5_w10_decode_snapshot(&decoded, w0);
    axis5_w10_absorb_snapshot(&replayed, &decoded);

    axis5_w10_decode_snapshot(&decoded, w1);
    axis5_w10_absorb_snapshot(&replayed, &decoded);

    axis5_w10_decode_snapshot(&decoded, w0);
    axis5_w10_absorb_snapshot(&replayed, &decoded);

    axis5_w10_decode_snapshot(&decoded, w2);
    axis5_w10_absorb_snapshot(&replayed, &decoded);

    axis5_w10_decode_snapshot(&decoded, w1);
    axis5_w10_absorb_snapshot(&replayed, &decoded);

    const unsigned int sig_direct = axis5_w10_aggregate_signature(&direct);
    const unsigned int sig_replayed = axis5_w10_aggregate_signature(&replayed);
    const unsigned int same = (sig_direct == sig_replayed) ? 1u : 0u;

    printf("%u %u %u\n", sig_direct, sig_replayed, same);
    return 0;
}
