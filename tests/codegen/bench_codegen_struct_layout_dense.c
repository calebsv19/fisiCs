typedef struct Vec2 {
    int x;
    int y;
} Vec2;

typedef struct Segment {
    Vec2 a;
    Vec2 b;
    int flags;
} Segment;

typedef union Payload {
    Segment seg;
    int raw[5];
} Payload;

typedef struct Bucket {
    Payload payloads[6];
    int bias;
} Bucket;

static Segment make_segment(int base) {
    Segment s = {
        .a = { base, base + 1 },
        .b = { base + 2, base + 3 },
        .flags = base ^ (base << 1)
    };
    return s;
}

static int checksum_bucket(Bucket* bucket) {
    int total = bucket->bias;
    for (int i = 0; i < 6; ++i) {
        bucket->payloads[i].seg = make_segment(i * 9 + bucket->bias);
        total += bucket->payloads[i].seg.a.x;
        total += bucket->payloads[i].seg.b.y;
        total ^= bucket->payloads[i].seg.flags;
    }
    return total;
}

int main(void) {
    Bucket bucket = {0};
    bucket.bias = 17;
    return checksum_bucket(&bucket);
}
