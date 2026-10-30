#define RETURN_HELPER
#include "15__units_return_contract.h"
#undef RETURN_HELPER
double travel(double speed FISICS_DIM(speed),double dt FISICS_DIM(time)) {
#if FAIL_KIND == 3
    return dt;
#else
    return speed*dt;
#endif
}
