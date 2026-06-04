#include <limits.h>

long long wave19_limits_bridge_width_score(void);

int main(void) {
    long long score = wave19_limits_bridge_width_score();
    return score >= (CHAR_BIT + 16 + 32) ? 0 : 1;
}
