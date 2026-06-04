#include <iso646.h>

int wave18_iso646_boolean_surface(int lhs, int rhs) {
    int both = (lhs and rhs) ? 1 : 0;
    int either = (lhs or rhs) ? 2 : 0;
    int inverted = (not lhs) ? 4 : 0;
    int differs = (lhs not_eq rhs) ? 8 : 0;

    return both + either + inverted + differs;
}
