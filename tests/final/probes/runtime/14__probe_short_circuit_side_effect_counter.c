#include <stdio.h>

static int bump(int* counter, int value) {
    *counter += 1;
    return value;
}

int main(void) {
    int calls = 0;
    int score = 0;

    score += (0 && bump(&calls, 7));
    score += (1 || bump(&calls, 9));
    score += (1 && bump(&calls, 3));
    score += (0 || bump(&calls, 5));
    score += ((bump(&calls, 1), 0) && bump(&calls, 11));
    score += ((bump(&calls, 1), 1) || bump(&calls, 13));
    score += (0 ? bump(&calls, 17) : bump(&calls, 2));

    printf("%d %d\n", calls, score);
    return 0;
}
