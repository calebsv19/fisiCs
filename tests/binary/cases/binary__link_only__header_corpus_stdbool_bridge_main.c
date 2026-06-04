#include <stdbool.h>

int wave21_stdbool_bridge_score(bool lhs, bool rhs, int salt);

int main(void) {
    int score = wave21_stdbool_bridge_score(true, false, 6);
    return score == 23 ? 0 : 1;
}
