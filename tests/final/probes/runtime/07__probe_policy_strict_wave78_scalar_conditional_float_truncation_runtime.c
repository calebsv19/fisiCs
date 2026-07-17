#include <stdio.h>

enum Delta {
    DELTA_NEG = -7,
    DELTA_POS = 12
};

int main(void) {
    unsigned short wide = 65000u;
    signed char negative = -4;
    double selected = 1 ? (double)wide / 8.0 : (double)negative;
    int trunc_negative = (int)-12.875;
    int trunc_selected = (int)(0 ? selected : -3.75);
    unsigned int conditional_unsigned = 0 ? (unsigned int)negative : (unsigned int)DELTA_POS;
    int comparison = DELTA_NEG < conditional_unsigned;

    printf("%.3f %d %d %u %d\n",
           selected,
           trunc_negative,
           trunc_selected,
           conditional_unsigned,
           comparison);
    return 0;
}
