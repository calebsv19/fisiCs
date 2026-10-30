#ifndef SOLVER_PHYSICS_INTERFACE_H
#define SOLVER_PHYSICS_INTERFACE_H
#include <fisics/extensions.h>
/* Explicit result contract: callers need no access to helper.c. */
#ifndef OMIT_RESULT_CONTRACT
#define TRAVEL_RESULT FISICS_DIM(length) FISICS_UNIT(meter)
#else
#define TRAVEL_RESULT
#endif
double TRAVEL_RESULT travel(double velocity FISICS_DIM(speed) FISICS_UNIT(meter_per_second),
              double dt FISICS_DIM(time) FISICS_UNIT(second));
#endif
