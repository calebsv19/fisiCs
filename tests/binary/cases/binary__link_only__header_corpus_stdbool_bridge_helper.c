#include <stdbool.h>

int wave21_stdbool_bridge_score(bool lhs, bool rhs, int salt) {
    int score = 0;
    score += lhs ? 11 : 0;
    score += rhs ? 7 : 0;
    score += (!rhs && lhs) ? 5 : 0;
    score += (bool)salt ? salt + 1 : 0;
    return score;
}
