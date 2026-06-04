#include <math.h>

int wave30_math_bridge_score(double value) {
    return (int)floor(value) + (int)ceil(value) + (int)trunc(value) + (int)round(value) + isfinite(value);
}
