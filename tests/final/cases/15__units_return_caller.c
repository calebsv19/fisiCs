#include <stdio.h>
#include "15__units_return_contract.h"
int main(void) {
    double speed FISICS_DIM(speed)=3;
    double dt FISICS_DIM(time)=2;
#if FAIL_KIND == 1 || FAIL_KIND == 4
    double result FISICS_DIM(time)=travel(speed,dt);
#elif FAIL_KIND == 2
    double result FISICS_DIM(time)=0;
    result=travel(speed,dt);
#else
    double result FISICS_DIM(length)=travel(speed,dt);
#endif
    printf("%.1f\n",result);
    return 0;
}
