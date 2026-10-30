/* Reduced from orbit_body_refresh_energy; local physical annotations are added
 * at raw body fields exactly as in the source customer. */
#include <fisics/extensions.h>
#include <stdio.h>
#include <math.h>
typedef struct { double mass_kg, velocity[2], kinetic_energy_j; } Body;
static void refresh(Body *body) {
    double mass FISICS_DIM(mass) FISICS_UNIT(kilogram) = body->mass_kg;
    double vx FISICS_DIM(speed) FISICS_UNIT(meter_per_second) = body->velocity[0];
    double vy FISICS_DIM(speed) FISICS_UNIT(meter_per_second) = body->velocity[1];
#if defined(BAD_ENERGY_ASSIGN)
    double energy FISICS_DIM(energy) FISICS_UNIT(joule) = 0;
    energy = 0.5 * mass * vx;
#elif defined(BAD_ENERGY)
    double energy FISICS_DIM(energy) FISICS_UNIT(joule) = 0.5 * mass * vx; /* missing velocity */
#else
    double energy FISICS_DIM(energy) FISICS_UNIT(joule) = 0.5 * mass * (vx*vx+vy*vy);
#endif
    body->kinetic_energy_j = energy;
#ifdef BAD_MATH_BOUNDARY
    double duration FISICS_DIM(time) = sqrt(vx*vx+vy*vy); /* desired: reject */
    body->kinetic_energy_j = duration;
#endif
}
int main(void) {
    Body body = {2,{3,4},0};
    refresh(&body);
    printf("%.9f\n",body.kinetic_energy_j);
    return 0;
}
