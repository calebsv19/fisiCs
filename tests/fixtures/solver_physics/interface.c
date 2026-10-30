#include "interface.h"
#include <stdio.h>
#ifdef SAME_TU
#include "helper.c"
#endif
int main(void) {
#ifdef BAD_ARGUMENT_DIM
    double velocity FISICS_DIM(time) FISICS_UNIT(second) = 3;
#elif defined(BAD_ARGUMENT_UNIT)
    double velocity FISICS_DIM(speed) FISICS_UNIT(foot_per_second) = 3;
#else
    double velocity FISICS_DIM(speed) FISICS_UNIT(meter_per_second) = 3;
#endif
    double dt FISICS_DIM(time) FISICS_UNIT(second) = 2;
#if defined(BAD_RETURN_ASSIGN)
    double result FISICS_DIM(time) = 0;
    result = travel(velocity,dt);
#elif defined(BAD_RETURN_CONSUMER)
    double result FISICS_DIM(time) = travel(velocity,dt); /* desired: reject */
#else
    double result FISICS_DIM(length) = travel(velocity,dt);
#endif
    printf("%.9f\n",result);
    return 0;
}
