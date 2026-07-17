#include <stdio.h>

static double wave89_conditional = 6.25;
static int wave89_relational = 1;
static double wave89_arithmetic = 7.5;

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
