#include <math.h>
#include <stdio.h>

int main(void) {
    static const double left[] = {7.0, 13.5, -9.0, 22.0};
    static const double right[] = {4.0, 2.0, 4.0, 5.0};
    long remainder_sum = 0;
    long fmod_sum = 0;
    long dim_sum = 0;
    long minmax_sum = 0;
    int quo_sum = 0;
    int sign_score = 0;
    int i = 0;
    long summary = 0;

    for (i = 0; i < 4; i++) {
        int quo = 0;
        double rem = remainder(left[i], right[i]);
        double remq = remquo(left[i], right[i], &quo);
        double mod = fmod(left[i], right[i]);
        double dim = fdim(left[i], right[i]);
        double lo = fmin(left[i], right[i]);
        double hi = fmax(left[i], right[i]);
        double signed_hi = copysign(hi, remq);

        remainder_sum += (long)(rem * 100.0) + (long)(remq * 10.0);
        fmod_sum += (long)(mod * 100.0);
        dim_sum += (long)(dim * 10.0);
        minmax_sum += (long)(lo * 10.0) + (long)(hi * 10.0);
        quo_sum += quo * (i + 1);
        sign_score += signbit(signed_hi) ? (i + 3) : (i + 7);
    }

    summary = remainder_sum + fmod_sum + dim_sum + minmax_sum + quo_sum +
              sign_score;

    printf("math-remainder-loop rem=%ld fmod=%ld dim=%ld minmax=%ld quo=%d sign=%d summary=%ld\n",
           remainder_sum,
           fmod_sum,
           dim_sum,
           minmax_sum,
           quo_sum,
           sign_score,
           summary);

    return remainder_sum == -55 && fmod_sum == 550 && dim_sum == 315 &&
                   minmax_sum == 485 && quo_sum == 26 && sign_score == 22 &&
                   summary == 1343
               ? 0
               : 1;
}
