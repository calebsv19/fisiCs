#include <math.h>
#include <stdio.h>

int main(void) {
    double pos = 2.75;
    double neg = -2.75;
    int a = (int)floor(pos);
    int b = (int)ceil(pos);
    int c = (int)trunc(pos);
    int d = (int)round(pos);
    int e = (int)floor(neg);
    int f = (int)ceil(neg);
    int g = (int)trunc(neg);
    int h = (int)round(neg);
    int total = a + b + c + d + e + f + g + h;

    printf("math-round vals=%d/%d/%d/%d/%d/%d/%d/%d total=%d\n",
           a,
           b,
           c,
           d,
           e,
           f,
           g,
           h,
           total);
    return a == 2 && b == 3 && c == 2 && d == 3 && e == -3 && f == -2 && g == -2 && h == -3 && total == 0 ? 0 : 1;
}
