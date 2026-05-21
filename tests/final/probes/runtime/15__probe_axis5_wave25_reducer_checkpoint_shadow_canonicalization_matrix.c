#include <stdio.h>

typedef struct Axis5W25ShadowRow {
    unsigned int lane;
    unsigned int checkpoint;
    unsigned int shadow;
    int delta;
} Axis5W25ShadowRow;

typedef struct Axis5W25ShadowAgg {
    unsigned int checkpoint[4];
    unsigned int shadow[4];
    int value[4];
} Axis5W25ShadowAgg;

static unsigned int axis5_w25_shadow_mix(unsigned int h, unsigned int v) {
    h ^= v + 0x9e3779b9u + (h << 6) + (h >> 2);
    return h;
}

static void axis5_w25_shadow_clear(Axis5W25ShadowAgg* a) {
    for (int i = 0; i < 4; ++i) {
        a->checkpoint[i] = 0u;
        a->shadow[i] = 0u;
        a->value[i] = 0;
    }
}

static void axis5_w25_shadow_absorb(Axis5W25ShadowAgg* a, const Axis5W25ShadowRow* row) {
    unsigned int lane = row->lane % 4u;
    if (row->checkpoint < a->checkpoint[lane]) return;
    if (row->checkpoint > a->checkpoint[lane]) {
        a->checkpoint[lane] = row->checkpoint;
        a->shadow[lane] = row->shadow;
        a->value[lane] = row->delta;
        return;
    }
    if (row->shadow > a->shadow[lane]) {
        a->shadow[lane] = row->shadow;
        a->value[lane] = row->delta;
    } else if (row->shadow == a->shadow[lane]) {
        a->value[lane] += row->delta;
    }
}

static unsigned int axis5_w25_shadow_signature(const Axis5W25ShadowAgg* a) {
    unsigned int h = 2166136261u;
    for (unsigned int lane = 0; lane < 4u; ++lane) {
        h = axis5_w25_shadow_mix(h, lane + 1u);
        h = axis5_w25_shadow_mix(h, a->checkpoint[lane]);
        h = axis5_w25_shadow_mix(h, a->shadow[lane]);
        h = axis5_w25_shadow_mix(h, (unsigned int)(a->value[lane] & 0xffff));
    }
    return h;
}

static void axis5_w25_shadow_absorb_batch(Axis5W25ShadowAgg* agg, const Axis5W25ShadowRow* rows, unsigned int count) {
    for (unsigned int i = 0; i < count; ++i) axis5_w25_shadow_absorb(agg, &rows[i]);
}

int main(void) {
    const Axis5W25ShadowRow shard_a[] = {{0u,2u,1u,4},{1u,3u,2u,7},{2u,2u,3u,5},{3u,1u,1u,6}};
    const Axis5W25ShadowRow shard_b[] = {{0u,5u,2u,3},{1u,3u,4u,-2},{2u,5u,1u,8},{3u,2u,2u,9}};
    const Axis5W25ShadowRow shard_c[] = {{0u,5u,4u,6},{1u,6u,2u,5},{2u,5u,3u,2},{3u,4u,4u,4}};
    const Axis5W25ShadowRow shard_d[] = {{0u,5u,4u,-1},{1u,6u,2u,1},{2u,6u,5u,3},{3u,4u,4u,-2}};
    Axis5W25ShadowAgg direct, left, right;
    axis5_w25_shadow_clear(&direct);
    axis5_w25_shadow_absorb_batch(&direct, shard_a, 4);
    axis5_w25_shadow_absorb_batch(&direct, shard_b, 4);
    axis5_w25_shadow_absorb_batch(&direct, shard_c, 4);
    axis5_w25_shadow_absorb_batch(&direct, shard_d, 4);
    axis5_w25_shadow_clear(&left);
    axis5_w25_shadow_absorb_batch(&left, shard_a, 4);
    axis5_w25_shadow_absorb_batch(&left, shard_b, 4);
    axis5_w25_shadow_absorb_batch(&left, shard_c, 4);
    axis5_w25_shadow_absorb_batch(&left, shard_d, 4);
    axis5_w25_shadow_clear(&right);
    axis5_w25_shadow_absorb_batch(&right, shard_c, 4);
    axis5_w25_shadow_absorb_batch(&right, shard_a, 4);
    axis5_w25_shadow_absorb_batch(&right, shard_d, 4);
    axis5_w25_shadow_absorb_batch(&right, shard_b, 4);
    {
        unsigned int sig_direct = axis5_w25_shadow_signature(&direct);
        unsigned int sig_left = axis5_w25_shadow_signature(&left);
        unsigned int sig_right = axis5_w25_shadow_signature(&right);
        unsigned int same = (sig_direct == sig_left && sig_direct == sig_right) ? 1u : 0u;
        printf("%u %u %u %u\n", sig_direct, sig_left, sig_right, same);
    }
    return 0;
}
