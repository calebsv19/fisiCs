/* Extracted interface pattern: velocity * timestep, definition in another TU. */
#include "interface.h"
double travel(double velocity FISICS_DIM(speed) FISICS_UNIT(meter_per_second),
              double dt FISICS_DIM(time) FISICS_UNIT(second)) {
    double distance FISICS_DIM(length) = velocity*dt;
    return distance;
}
