#include <stdbool.h>
#include <stdio.h>

int main(void) {
    bool a = (bool)0;
    bool b = (bool)-7;
    bool c = true;
    bool d = false;
    int score = 0;

    score += a ? 100 : 1;
    score += b ? 10 : 200;
    score += c ? 3 : 300;
    score += d ? 400 : 5;
    score += ((int)b == 1) ? 7 : 0;

    printf("stdbool-conversion score=%d true=%d false=%d\n", score, (int)true, (int)false);
    return score == 26 ? 0 : 1;
}
