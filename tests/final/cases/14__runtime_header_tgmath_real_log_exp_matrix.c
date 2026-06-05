#include <stdio.h>
#include <tgmath.h>

int main(void) {
    double roundtrip = exp(log(8.0));
    double logten = log10(1000.0);
    double rem = fmod(17.5, 5.0);
    double maximum = fmax(-2.0, 6.0);
    double minimum = fmin(-2.0, 6.0);
    long roundtrip100 = (long)(roundtrip * 100.0 + 0.5);
    long logten100 = (long)(logten * 100.0 + 0.5);
    long rem100 = (long)(rem * 100.0 + 0.5);
    long maximum100 = (long)(maximum * 100.0 + 0.5);
    long minimum100 = (long)(minimum * 100.0 + (minimum >= 0.0 ? 0.5 : -0.5));
    long summary = roundtrip100 + logten100 + rem100 + maximum100 + minimum100;

    printf("tgmath-real-log exp=%ld log10=%ld fmod=%ld max=%ld min=%ld summary=%ld\n",
           roundtrip100,
           logten100,
           rem100,
           maximum100,
           minimum100,
           summary);

    return roundtrip100 == 800L && logten100 == 300L && rem100 == 250L &&
                   maximum100 == 600L && minimum100 == -200L && summary == 1750L
               ? 0
               : 1;
}
