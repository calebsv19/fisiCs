#include <iso646.h>
#include <stdio.h>

static int wave18_iso646_decide(int lhs, int rhs) {
    int score = 0;

    if (lhs and rhs) {
        score += 3;
    }
    if (lhs or rhs) {
        score += 5;
    }
    if (not (lhs and rhs)) {
        score += 7;
    }
    if (lhs not_eq rhs) {
        score += 11;
    }
    return score;
}

int main(void) {
    int total = wave18_iso646_decide(1, 0) * 10 + wave18_iso646_decide(1, 1);

    printf("iso646-boolean total=%d\n", total);
    return total == 238 ? 0 : 1;
}
