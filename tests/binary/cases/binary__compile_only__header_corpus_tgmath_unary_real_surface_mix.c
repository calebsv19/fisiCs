#include <tgmath.h>

struct Wave14TgmathUnaryPack {
    double abs_lane;
    double ceil_lane;
    double floor_lane;
    double root_lane;
    double tanh_lane;
};

struct Wave14TgmathUnaryPack wave14_tgmath_unary_pack_build(float seed, int lane) {
    struct Wave14TgmathUnaryPack pack;

    pack.abs_lane = fabs(-seed);
    pack.ceil_lane = ceil(seed + 0.25f);
    pack.floor_lane = floor(seed + 0.75f);
    pack.root_lane = sqrt((double)(lane * lane + 1));
    pack.tanh_lane = tanh((double)lane / 4.0);
    return pack;
}
