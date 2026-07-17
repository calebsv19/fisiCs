#include <stdio.h>

static int wave89_discarded_call(void) {
    return 99;
}

static double wave89_conditional =
    1 ? 6.25 : wave89_discarded_call();
static int wave89_relational = 1.25 < 2.5;
static double wave89_arithmetic = (1.5 + 2.25) * 2.0;

int main(void) {
    int conditional_hundredths = (int)(wave89_conditional * 100.0);
    int arithmetic_hundredths = (int)(wave89_arithmetic * 100.0);

    printf("%d %d %d\n",
           conditional_hundredths,
           wave89_relational,
           arithmetic_hundredths);
    return (conditional_hundredths == 625 &&
            wave89_relational == 1 &&
            arithmetic_hundredths == 750) ? 0 : 89;
}
