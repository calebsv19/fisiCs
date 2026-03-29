#include <stdio.h>

struct ld_lane {
    long double value;
    int tag;
};

static unsigned fold_bytes(const unsigned char *bytes, int n) {
    unsigned hash = 2166136261u;
    for (int i = 0; i < n; ++i) {
        hash = (hash ^ bytes[i]) * 16777619u;
    }
    return hash;
}

static struct ld_lane lane_forward(struct ld_lane in) {
    return in;
}

int main(void) {
    struct ld_lane lane;
    lane.value = 2.0L;
    lane.tag = 7;
    struct ld_lane out = lane_forward(lane);
    unsigned hash = fold_bytes((const unsigned char *)&out.value, (int)sizeof(out.value));
    printf("%zu %u %d\n", sizeof(struct ld_lane), hash, out.tag);
    return 0;
}
