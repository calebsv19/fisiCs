#include <string.h>

int wave87_complex_checksum(float _Complex value) {
    float lanes[2];
    int real_lane;
    int imaginary_lane;

    memcpy(lanes, &value, sizeof(value));
    real_lane = (int)(lanes[0] * 4.0f);
    imaginary_lane = (int)(lanes[1] * 4.0f);
    return real_lane * 100 + imaginary_lane;
}
