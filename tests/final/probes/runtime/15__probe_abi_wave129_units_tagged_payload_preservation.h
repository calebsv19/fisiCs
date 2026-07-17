union Wave129AbiBits {
    unsigned words[4];
    struct {
        unsigned lo;
        unsigned hi;
        unsigned tag;
        unsigned mask;
    } named;
};

struct Wave129AbiUnitPayload {
    double lanes[3];
    union Wave129AbiBits bits;
    unsigned route;
};

typedef struct Wave129AbiUnitPayload (*Wave129AbiStep)(struct Wave129AbiUnitPayload, unsigned);

struct Wave129AbiUnitPayload wave129_abi_seed(double feet, double wh, unsigned seed);
struct Wave129AbiUnitPayload wave129_abi_mix(struct Wave129AbiUnitPayload payload, unsigned step);
unsigned wave129_abi_fold(struct Wave129AbiUnitPayload payload, unsigned salt);
