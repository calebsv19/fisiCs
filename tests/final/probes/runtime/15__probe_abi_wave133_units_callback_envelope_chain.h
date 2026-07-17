union Wave133ChainPayload {
    unsigned words[6];
    struct {
        unsigned lane;
        unsigned route;
        unsigned tag;
        unsigned mix;
        unsigned generation;
        unsigned owner;
    } named;
};

struct Wave133ChainCell {
    double meters;
    double joules;
    double coulombs;
    double seconds;
    union Wave133ChainPayload payload;
};

struct Wave133ChainSegment {
    struct Wave133ChainCell cells[2];
    union Wave133ChainPayload segment_footer;
    unsigned epoch;
};

struct Wave133ChainEnvelope {
    struct Wave133ChainSegment segments[2][2];
    union Wave133ChainPayload envelope_footer;
    unsigned generation;
};

typedef struct Wave133ChainSegment (*Wave133SegmentFn)(struct Wave133ChainSegment, struct Wave133ChainCell, unsigned);

struct Wave133ChainEnvelope wave133_chain_seed(double feet, double watt_hours, double milliamp_hours, double millis, unsigned seed);
struct Wave133ChainSegment wave133_chain_bias_segment(struct Wave133ChainSegment segment, struct Wave133ChainCell carry, unsigned step);
struct Wave133ChainSegment wave133_chain_cross_segment(struct Wave133ChainSegment segment, struct Wave133ChainCell carry, unsigned step);
struct Wave133ChainEnvelope wave133_chain_apply(struct Wave133ChainEnvelope envelope, Wave133SegmentFn callback, unsigned step);
unsigned wave133_chain_digest(struct Wave133ChainEnvelope envelope, unsigned salt);
