#include <math.h>
#include <stdio.h>

int main(void) {
    static const double values[] = {0.5, 1.0, 2.0, 3.5, 16.0, 64.0};
    int ilog_sum = 0;
    long logb_sum = 0;
    long scale_sum = 0;
    long mant_sum = 0;
    int exp_sum = 0;
    int i = 0;
    long summary = 0;

    for (i = 0; i < 6; i++) {
        int exp_part = 0;
        double mant = frexp(values[i], &exp_part);
        double scaled = scalbn(mant, exp_part + (i % 3) - 1);
        double scaled_long = scalbln(values[i], (long)((i % 2) + 1));
        ilog_sum += ilogb(values[i]) * (i + 1);
        logb_sum += (long)(logb(values[i]) * 10.0);
        scale_sum += (long)(scaled * 100.0) + (long)(scaled_long * 10.0);
        mant_sum += (long)(mant * 1000.0);
        exp_sum += exp_part * (i + 3);
    }

    summary = (long)ilog_sum + logb_sum + scale_sum + mant_sum + exp_sum;

    printf("math-exponent-loop ilog=%d logb=%ld scale=%ld mant=%ld exp=%d summary=%ld\n",
           ilog_sum,
           logb_sum,
           scale_sum,
           mant_sum,
           exp_sum,
           summary);

    return ilog_sum == 62 && logb_sum == 110 && scale_sum == 18210 &&
                   mant_sum == 3375 && exp_sum == 117 && summary == 21874
               ? 0
               : 1;
}
