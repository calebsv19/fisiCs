#include <limits.h>

long long wave19_limits_bridge_width_score(void) {
    return (long long)CHAR_BIT +
           (long long)(sizeof(short) * CHAR_BIT) +
           (long long)(sizeof(int) * CHAR_BIT) +
           (long long)(sizeof(long) * CHAR_BIT) +
           (long long)(sizeof(long long) * CHAR_BIT);
}
