#include <math.h>
#include <stdio.h>

int main(void) {
    double up = nextafter(1.0, 2.0);
    double down = nextafter(1.0, 0.0);
    double fused = fma(2.0, 3.0, 0.5);
    double root = sqrt(81.0);
    double cube = cbrt(27.0);
    int up_ok = up > 1.0 && up < 2.0;
    int down_ok = down < 1.0 && down > 0.0;
    int fused10 = (int)(fused * 10.0);
    int root10 = (int)(root * 10.0);
    int cube10 = (int)(cube * 10.0);
    int summary = up_ok + down_ok * 3 + fused10 + root10 + cube10;

    printf("math-next up=%d down=%d fused10=%d root10=%d cube10=%d summary=%d\n",
           up_ok,
           down_ok,
           fused10,
           root10,
           cube10,
           summary);

    return up_ok == 1 && down_ok == 1 && fused10 == 65 && root10 == 90 &&
                   cube10 == 30 && summary == 189
               ? 0
               : 1;
}
