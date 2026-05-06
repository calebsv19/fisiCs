#include <stdio.h>

typedef struct Axis5W11Snapshot {
    unsigned int shard;
    unsigned int watermark;
    unsigned int epoch;
    unsigned int bins[2];
} Axis5W11Snapshot;

typedef struct Axis5W11Aggregate {
    unsigned int watermark_by_shard[2];
    unsigned int epoch_by_shard[2];
    unsigned int bins_by_shard[2][2];
} Axis5W11Aggregate;

void axis5_w11_seed_snapshot(
    Axis5W11Snapshot* out,
    unsigned int shard,
    unsigned int watermark,
    unsigned int epoch,
    unsigned int a,
    unsigned int b
);
void axis5_w11_encode_snapshot(const Axis5W11Snapshot* s, unsigned int wire[5]);
void axis5_w11_decode_snapshot(Axis5W11Snapshot* s, const unsigned int wire[5]);
void axis5_w11_clear_aggregate(Axis5W11Aggregate* a);
void axis5_w11_absorb_snapshot(Axis5W11Aggregate* a, const Axis5W11Snapshot* s);
unsigned int axis5_w11_signature(const Axis5W11Aggregate* a);

int main(void) {
    Axis5W11Snapshot s0_new;
    Axis5W11Snapshot s0_old;
    Axis5W11Snapshot s1_mid;
    Axis5W11Snapshot s1_new;
    Axis5W11Snapshot decoded;
    Axis5W11Aggregate direct;
    Axis5W11Aggregate replayed;
    unsigned int w0_new[5];
    unsigned int w0_old[5];
    unsigned int w1_mid[5];
    unsigned int w1_new[5];

    axis5_w11_seed_snapshot(&s0_new, 0u, 3u, 5u, 11u, 7u);
    axis5_w11_seed_snapshot(&s0_old, 0u, 2u, 6u, 4u, 1u);
    axis5_w11_seed_snapshot(&s1_mid, 1u, 2u, 4u, 8u, 3u);
    axis5_w11_seed_snapshot(&s1_new, 1u, 4u, 3u, 12u, 9u);

    axis5_w11_encode_snapshot(&s0_new, w0_new);
    axis5_w11_encode_snapshot(&s0_old, w0_old);
    axis5_w11_encode_snapshot(&s1_mid, w1_mid);
    axis5_w11_encode_snapshot(&s1_new, w1_new);

    axis5_w11_clear_aggregate(&direct);
    axis5_w11_absorb_snapshot(&direct, &s0_new);
    axis5_w11_absorb_snapshot(&direct, &s1_new);

    axis5_w11_clear_aggregate(&replayed);
    axis5_w11_decode_snapshot(&decoded, w0_old);
    axis5_w11_absorb_snapshot(&replayed, &decoded);
    axis5_w11_decode_snapshot(&decoded, w1_mid);
    axis5_w11_absorb_snapshot(&replayed, &decoded);
    axis5_w11_decode_snapshot(&decoded, w0_new);
    axis5_w11_absorb_snapshot(&replayed, &decoded);
    axis5_w11_decode_snapshot(&decoded, w1_new);
    axis5_w11_absorb_snapshot(&replayed, &decoded);
    axis5_w11_decode_snapshot(&decoded, w1_mid);
    axis5_w11_absorb_snapshot(&replayed, &decoded);
    axis5_w11_decode_snapshot(&decoded, w0_old);
    axis5_w11_absorb_snapshot(&replayed, &decoded);

    {
        unsigned int sig_direct = axis5_w11_signature(&direct);
        unsigned int sig_replayed = axis5_w11_signature(&replayed);
        unsigned int same = (sig_direct == sig_replayed) ? 1u : 0u;
        printf("%u %u %u\n", sig_direct, sig_replayed, same);
    }
    return 0;
}
