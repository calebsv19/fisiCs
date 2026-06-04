#include <tgmath.h>

struct Wave14TgmathBinaryPack {
    double pow_lane;
    double hypot_lane;
    double max_lane;
    double min_lane;
    double angle_lane;
};

struct Wave14TgmathBinaryPack wave14_tgmath_binary_pack_build(float left, double right, int lane) {
    struct Wave14TgmathBinaryPack pack;

    pack.pow_lane = pow(left + 1.0f, 2.0);
    pack.hypot_lane = hypot(left, right);
    pack.max_lane = fmax(left + (float)lane, right);
    pack.min_lane = fmin(left - 1.0f, right);
    pack.angle_lane = atan2(right + 1.0, (double)lane + 1.0);
    return pack;
}
