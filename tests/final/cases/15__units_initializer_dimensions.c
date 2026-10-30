/* Stable P2.1 CLI/codegen gate; exact diagnostic codes/counts and the broader
 * initializer shape matrix are owned by frontend_api_contract_units_initializer. */
#include "../../../include/fisics/extensions.h"
#include <stdio.h>
#ifndef FAIL_KIND
#define FAIL_KIND 0
#endif
static double distance(void) {
    double value FISICS_DIM(length) = 6;
    return value;
}
int main(void) {
    double dt FISICS_DIM(time) = 2;
#if FAIL_KIND == 1
    double wrong FISICS_DIM(length) = dt;
    (void)wrong;
#elif FAIL_KIND == 2
    double wrong[2] FISICS_DIM(length) = {0,dt};
    (void)wrong;
#elif FAIL_KIND == 3
    struct S { double value FISICS_DIM(length); };
    struct S wrong = {.value=dt};
    (void)wrong;
#elif FAIL_KIND == 4
    double wrong FISICS_DIM(time) = distance();
    (void)wrong;
#else
    double negative FISICS_DIM(length) = -2;
    double cast_value FISICS_DIM(time) = (double)4;
    double fraction FISICS_DIM(time) = 1.0/2;
    double zero FISICS_DIM(length) = 0;
    double values[2] FISICS_DIM(length) = {-1,3};
    double known FISICS_DIM(length) = distance();
    printf("%.1f %.1f %.1f %.1f %.1f %.1f %.1f\n", negative,cast_value,
           fraction,zero,values[0],values[1],known);
#endif
    (void)dt;
    return 0;
}
