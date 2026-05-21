#include <stdio.h>

typedef struct Axis6W1ResumeRow {
    unsigned int lane;
    unsigned int phase;
    unsigned int budget;
    unsigned int token;
    int delta;
} Axis6W1ResumeRow;

typedef struct Axis6W1ResumeAgg {
    unsigned int phase[4];
    unsigned int budget[4];
    unsigned int seen_mask[4];
    int value[4];
} Axis6W1ResumeAgg;

static unsigned int axis6_w1_resume_mix(unsigned int h, unsigned int v) {
    h ^= v + 0x9e3779b9u + (h << 6) + (h >> 2);
    return h;
}

static void axis6_w1_resume_clear(Axis6W1ResumeAgg* a) {
    for (int i = 0; i < 4; ++i) {
        a->phase[i] = 0u;
        a->budget[i] = 0u;
        a->seen_mask[i] = 0u;
        a->value[i] = 0;
    }
}

static void axis6_w1_resume_absorb(Axis6W1ResumeAgg* a, const Axis6W1ResumeRow* row) {
    unsigned int lane = row->lane % 4u;
    if (row->phase < a->phase[lane]) return;
    if (row->phase > a->phase[lane]) {
        a->phase[lane] = row->phase;
        a->budget[lane] = row->budget;
        a->seen_mask[lane] = 1u << (row->token & 31u);
        a->value[lane] = row->delta;
        return;
    }
    if (row->budget < a->budget[lane]) return;
    if (row->budget > a->budget[lane]) {
        a->budget[lane] = row->budget;
        a->seen_mask[lane] = 1u << (row->token & 31u);
        a->value[lane] = row->delta;
        return;
    }
    {
        unsigned int bit = 1u << (row->token & 31u);
        if (a->seen_mask[lane] & bit) return;
        a->seen_mask[lane] |= bit;
    }
    a->value[lane] += row->delta;
}

static unsigned int axis6_w1_resume_signature(const Axis6W1ResumeAgg* a) {
    unsigned int h = 2166136261u;
    for (unsigned int lane = 0; lane < 4u; ++lane) {
        h = axis6_w1_resume_mix(h, lane + 1u);
        h = axis6_w1_resume_mix(h, a->phase[lane]);
        h = axis6_w1_resume_mix(h, a->budget[lane]);
        h = axis6_w1_resume_mix(h, a->seen_mask[lane]);
        h = axis6_w1_resume_mix(h, (unsigned int)(a->value[lane] & 0xffff));
    }
    return h;
}

static void axis6_w1_resume_absorb_batch(Axis6W1ResumeAgg* agg, const Axis6W1ResumeRow* rows, unsigned int count) {
    for (unsigned int i = 0; i < count; ++i) axis6_w1_resume_absorb(agg, &rows[i]);
}

int main(void) {
    const Axis6W1ResumeRow canonical[] = {
        {0u,4u,2u,11u,5}, {0u,4u,2u,17u,-1},
        {1u,5u,3u,13u,4}, {1u,5u,3u,19u,2},
        {2u,6u,4u,23u,7}, {2u,6u,4u,29u,-3},
        {3u,4u,5u,31u,6}, {3u,4u,5u,5u,1}
    };
    const Axis6W1ResumeRow replay_a[] = {
        {0u,2u,1u,3u,1}, {0u,4u,2u,11u,5}, {1u,3u,2u,7u,2}, {2u,6u,4u,23u,7},
        {1u,5u,3u,13u,4}, {0u,4u,2u,11u,5}, {3u,4u,5u,31u,6}, {2u,6u,4u,29u,-3},
        {1u,5u,3u,19u,2}, {3u,4u,5u,31u,6}, {0u,4u,2u,17u,-1}, {3u,4u,5u,5u,1}
    };
    const Axis6W1ResumeRow replay_b[] = {
        {3u,1u,2u,9u,4}, {3u,4u,5u,5u,1}, {1u,5u,3u,19u,2}, {0u,4u,2u,17u,-1},
        {2u,6u,4u,23u,7}, {1u,5u,3u,13u,4}, {0u,4u,2u,11u,5}, {3u,4u,5u,31u,6},
        {2u,6u,4u,29u,-3}, {0u,4u,2u,17u,-1}, {1u,5u,3u,19u,2}, {3u,4u,5u,5u,1}
    };
    Axis6W1ResumeAgg direct, dedup_a, dedup_b;
    axis6_w1_resume_clear(&direct);
    axis6_w1_resume_absorb_batch(&direct, canonical, sizeof(canonical) / sizeof(canonical[0]));
    axis6_w1_resume_clear(&dedup_a);
    axis6_w1_resume_absorb_batch(&dedup_a, replay_a, sizeof(replay_a) / sizeof(replay_a[0]));
    axis6_w1_resume_clear(&dedup_b);
    axis6_w1_resume_absorb_batch(&dedup_b, replay_b, sizeof(replay_b) / sizeof(replay_b[0]));
    {
        unsigned int sig_direct = axis6_w1_resume_signature(&direct);
        unsigned int sig_a = axis6_w1_resume_signature(&dedup_a);
        unsigned int sig_b = axis6_w1_resume_signature(&dedup_b);
        unsigned int same = (sig_direct == sig_a && sig_direct == sig_b) ? 1u : 0u;
        printf("%u %u %u %u\n", sig_direct, sig_a, sig_b, same);
    }
    return 0;
}
