#include <stdio.h>

typedef struct Axis5W25Snapshot {
    unsigned int shard;
    unsigned int epoch;
    unsigned int watermark;
    unsigned int lane_a;
    unsigned int lane_b;
} Axis5W25Snapshot;

typedef struct Axis5W25Aggregate {
    unsigned int epoch[4];
    unsigned int watermark[4];
    unsigned int lane_a[4];
    unsigned int lane_b[4];
} Axis5W25Aggregate;

void axis5_w25_seed_snapshot(Axis5W25Snapshot* out, unsigned int shard, unsigned int epoch, unsigned int watermark, unsigned int lane_a, unsigned int lane_b);
void axis5_w25_encode_snapshot(const Axis5W25Snapshot* s, unsigned int wire[5]);
void axis5_w25_decode_snapshot(Axis5W25Snapshot* s, const unsigned int wire[5]);
void axis5_w25_clear_aggregate(Axis5W25Aggregate* a);
void axis5_w25_absorb_snapshot(Axis5W25Aggregate* a, const Axis5W25Snapshot* s);
unsigned int axis5_w25_snapshot_signature(const Axis5W25Aggregate* a);

int main(void) {
    Axis5W25Snapshot s0_old,s0_new,s1_old,s1_new,s2_old,s2_new,s3_old,s3_new,decoded;
    Axis5W25Aggregate canonical,replayed;
    unsigned int w0_old[5],w0_new[5],w1_old[5],w1_new[5],w2_old[5],w2_new[5],w3_old[5],w3_new[5];
    axis5_w25_seed_snapshot(&s0_old,0u,3u,2u,5u,6u);
    axis5_w25_seed_snapshot(&s0_new,0u,7u,5u,9u,11u);
    axis5_w25_seed_snapshot(&s1_old,1u,2u,1u,4u,7u);
    axis5_w25_seed_snapshot(&s1_new,1u,8u,6u,10u,12u);
    axis5_w25_seed_snapshot(&s2_old,2u,4u,3u,8u,5u);
    axis5_w25_seed_snapshot(&s2_new,2u,9u,7u,14u,15u);
    axis5_w25_seed_snapshot(&s3_old,3u,5u,2u,3u,9u);
    axis5_w25_seed_snapshot(&s3_new,3u,10u,8u,16u,13u);
    axis5_w25_encode_snapshot(&s0_old,w0_old); axis5_w25_encode_snapshot(&s0_new,w0_new);
    axis5_w25_encode_snapshot(&s1_old,w1_old); axis5_w25_encode_snapshot(&s1_new,w1_new);
    axis5_w25_encode_snapshot(&s2_old,w2_old); axis5_w25_encode_snapshot(&s2_new,w2_new);
    axis5_w25_encode_snapshot(&s3_old,w3_old); axis5_w25_encode_snapshot(&s3_new,w3_new);
    axis5_w25_clear_aggregate(&canonical);
    axis5_w25_absorb_snapshot(&canonical,&s0_new);
    axis5_w25_absorb_snapshot(&canonical,&s1_new);
    axis5_w25_absorb_snapshot(&canonical,&s2_new);
    axis5_w25_absorb_snapshot(&canonical,&s3_new);
    axis5_w25_clear_aggregate(&replayed);
    axis5_w25_decode_snapshot(&decoded,w2_old); axis5_w25_absorb_snapshot(&replayed,&decoded);
    axis5_w25_decode_snapshot(&decoded,w1_new); axis5_w25_absorb_snapshot(&replayed,&decoded);
    axis5_w25_decode_snapshot(&decoded,w0_old); axis5_w25_absorb_snapshot(&replayed,&decoded);
    axis5_w25_decode_snapshot(&decoded,w3_new); axis5_w25_absorb_snapshot(&replayed,&decoded);
    axis5_w25_decode_snapshot(&decoded,w2_new); axis5_w25_absorb_snapshot(&replayed,&decoded);
    axis5_w25_decode_snapshot(&decoded,w1_old); axis5_w25_absorb_snapshot(&replayed,&decoded);
    axis5_w25_decode_snapshot(&decoded,w0_new); axis5_w25_absorb_snapshot(&replayed,&decoded);
    axis5_w25_decode_snapshot(&decoded,w3_old); axis5_w25_absorb_snapshot(&replayed,&decoded);
    {
        unsigned int sig_canonical = axis5_w25_snapshot_signature(&canonical);
        unsigned int sig_replayed = axis5_w25_snapshot_signature(&replayed);
        unsigned int same = (sig_canonical == sig_replayed) ? 1u : 0u;
        printf("%u %u %u\n", sig_canonical, sig_replayed, same);
    }
    return 0;
}
