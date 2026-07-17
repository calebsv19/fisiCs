union Wave131Payload {
    unsigned words[4];
    struct {
        unsigned lane;
        unsigned tag;
        unsigned mix;
        unsigned route;
    } named;
};

struct Wave131Sample {
    double meters;
    double joules;
    double coulombs;
    union Wave131Payload payload;
};

struct Wave131Packet {
    struct Wave131Sample samples[2][2];
    unsigned epoch;
};

typedef struct Wave131Sample (*Wave131SampleFn)(struct Wave131Sample, unsigned);

struct Wave131Packet wave131_seed_packet(double feet, double watt_hours, double milliamp_hours, unsigned seed);
struct Wave131Sample wave131_bias_sample(struct Wave131Sample sample, unsigned step);
struct Wave131Sample wave131_cross_sample(struct Wave131Sample sample, unsigned step);
struct Wave131Packet wave131_apply_packet(struct Wave131Packet packet, Wave131SampleFn callback, unsigned step);
unsigned wave131_packet_digest(struct Wave131Packet packet, unsigned salt);
