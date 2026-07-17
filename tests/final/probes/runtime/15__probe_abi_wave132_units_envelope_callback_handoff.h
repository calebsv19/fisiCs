union Wave132Payload {
    unsigned words[6];
    struct {
        unsigned lane;
        unsigned tag;
        unsigned checksum;
        unsigned route;
        unsigned generation;
        unsigned owner;
    } named;
};

struct Wave132Cell {
    double meters;
    double joules;
    double coulombs;
    double seconds;
    union Wave132Payload payload;
};

struct Wave132Frame {
    struct Wave132Cell cells[3];
    unsigned epoch;
};

struct Wave132Envelope {
    struct Wave132Frame frames[2];
    union Wave132Payload footer;
    unsigned generation;
};

typedef struct Wave132Frame (*Wave132FrameFn)(struct Wave132Frame, struct Wave132Cell, unsigned);

struct Wave132Envelope wave132_seed_envelope(double feet, double watt_hours, double milliamp_hours, double millis, unsigned seed);
struct Wave132Frame wave132_bias_frame(struct Wave132Frame frame, struct Wave132Cell carry, unsigned step);
struct Wave132Frame wave132_shear_frame(struct Wave132Frame frame, struct Wave132Cell carry, unsigned step);
struct Wave132Envelope wave132_apply_envelope(struct Wave132Envelope envelope, Wave132FrameFn callback, unsigned step);
unsigned wave132_envelope_digest(struct Wave132Envelope envelope, unsigned salt);
