#include <stdio.h>

enum Scale {
    SCALE_SMALL = 3,
    SCALE_LARGE = 9
};

struct Sample {
    unsigned char raw;
    signed char bias;
};

struct Bucket {
    enum Scale scale;
    struct Sample samples[3];
};

typedef int (*sample_fn)(struct Bucket *, int, unsigned short);

static int score_sample(struct Bucket *bucket, int index, unsigned short salt) {
    struct Sample *sample = &bucket->samples[index];
    unsigned int widened = (unsigned int)(unsigned char)(sample->raw + (unsigned char)salt);
    int signed_bias = (int)(signed char)(sample->bias - (signed char)bucket->scale);
    return (int)(unsigned char)widened + signed_bias;
}

static int apply(sample_fn fn, struct Bucket *bucket, int index, unsigned short salt) {
    sample_fn selected = index > 1 ? score_sample : fn;
    return selected(bucket, index, salt);
}

int main(void) {
    struct Bucket buckets[2] = {
        {SCALE_SMALL, {{250u, -4}, {12u, 5}, {99u, -12}}},
        {SCALE_LARGE, {{44u, 7}, {201u, -8}, {31u, 11}}}
    };

    struct Bucket *chosen = 1 ? &buckets[0] : &buckets[1];
    sample_fn fn = score_sample;
    int first = apply(fn, chosen, 0, 9u);
    int second = apply(fn, &buckets[1], 1, 17u);
    int third = apply(fn, &buckets[0], 2, 33u);
    printf("%d %d %d\n", first, second, third);
    return 0;
}
