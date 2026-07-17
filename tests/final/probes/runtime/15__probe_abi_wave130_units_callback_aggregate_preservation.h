union Wave130CellBits {
    unsigned words[4];
    struct {
        unsigned lo;
        unsigned hi;
        unsigned tag;
        unsigned route;
    } named;
};

struct Wave130Cell {
    double distance_m;
    double reserve_j;
    double charge_c;
    union Wave130CellBits bits;
};

struct Wave130Frame {
    struct Wave130Cell cells[2];
    unsigned epoch;
};

typedef struct Wave130Cell (*Wave130CellFn)(struct Wave130Cell, unsigned);

struct Wave130Frame wave130_seed_frame(double feet, double watt_hours, double milliamp_hours, unsigned seed);
struct Wave130Cell wave130_adjust_cell(struct Wave130Cell cell, unsigned step);
struct Wave130Cell wave130_fold_callback(struct Wave130Cell cell, unsigned step);
struct Wave130Frame wave130_apply_frame(struct Wave130Frame frame, Wave130CellFn callback, unsigned step);
unsigned wave130_frame_digest(struct Wave130Frame frame, unsigned salt);
