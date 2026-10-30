/* P2.2 CLI/runtime controls; precise codes and more callable forms are covered
 * by frontend_api_contract_units_arguments. */
#include "../../../include/fisics/extensions.h"
#include <stdio.h>
#ifndef FAIL_KIND
#define FAIL_KIND 0
#endif
static double accept(double value FISICS_DIM(length) FISICS_UNIT(meter)) {
    return value;
}
typedef double Fn(double value FISICS_DIM(length) FISICS_UNIT(meter));
static void pair(double length FISICS_DIM(length), double duration FISICS_DIM(time)) {
    (void)length; (void)duration;
}
int main(void) {
    double dt FISICS_DIM(time) = 2;
    double length FISICS_DIM(length) FISICS_UNIT(meter) = 3;
#if FAIL_KIND == 1
    (void)accept(dt);
#elif FAIL_KIND == 2
    Fn *fn=accept;
    (void)fn(dt);
#elif FAIL_KIND == 3
    struct S { Fn *fn; } s={accept};
    (void)s.fn(dt);
#elif FAIL_KIND == 4
    double ratio FISICS_DIM(scalar) = 1;
    (void)accept(ratio);
#elif FAIL_KIND == 5
    pair(length,length);
#else
    Fn *fn=accept;
    double result FISICS_DIM(length) = fn(length);
    pair(length,dt);
    printf("%.1f %.1f %.1f %.1f %.1f %.1f\n",result,accept(3),accept(-2),
           accept((double)2),accept(1.0/2),accept(0));
#endif
    (void)dt; (void)length;
    return 0;
}
