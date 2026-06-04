#include <limits.h>

enum {
    wave19_limits_char_bit_ok = 1 / (CHAR_BIT >= 8),
    wave19_limits_schar_order_ok = 1 / (SCHAR_MIN < 0),
    wave19_limits_uchar_order_ok = 1 / (UCHAR_MAX >= 255)
};

int wave19_limits_char_surface(unsigned char value) {
    int promoted = (int)value;
    return promoted + CHAR_BIT + (int)(UCHAR_MAX & 0xffu);
}
