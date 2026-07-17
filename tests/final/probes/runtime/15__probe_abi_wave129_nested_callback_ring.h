union Wave129RingBits {
    unsigned words[3];
    struct {
        unsigned lo;
        unsigned hi;
        unsigned tag;
    } named;
};

struct Wave129RingLeaf {
    long bias;
    union Wave129RingBits bits;
};

struct Wave129RingPayload {
    struct Wave129RingLeaf leaves[2][3];
    unsigned epoch;
};

typedef struct Wave129RingPayload (*Wave129RingCallback)(struct Wave129RingPayload, unsigned);

struct Wave129RingPayload wave129_ring_seed(unsigned seed, long bias);
struct Wave129RingPayload wave129_ring_step(struct Wave129RingPayload payload,
                                            Wave129RingCallback callback,
                                            unsigned step);
unsigned wave129_ring_fold(struct Wave129RingPayload payload, unsigned salt);
