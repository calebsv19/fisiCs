#include <stdio.h>

#include <fisics/extensions.h>

#include "collision_1d.h"

int main(void) {
    CollisionBody1D body_a = {
        .mass_kg = 2.0,
        .velocity_mps = 3.0
    };
    CollisionBody1D body_b = {
        .mass_kg = 1.0,
        .velocity_mps = -5.0

    };

    double mass_a FISICS_DIM(mass) FISICS_UNIT(kilogram) = body_a.mass_kg;
    double mass_b FISICS_DIM(mass) FISICS_UNIT(kilogram) = body_b.mass_kg;
    double velocity_a FISICS_DIM(speed) FISICS_UNIT(meter_per_second) = body_a.velocity_mps;
    double velocity_b FISICS_DIM(speed) FISICS_UNIT(meter_per_second) = body_b.velocity_mps;

    collision_1d_elastic(&body_a, &body_b);

    velocity_a = body_a.velocity_mps;
    velocity_b = body_b.velocity_mps;

    double momentum_a FISICS_DIM(kg*m/s) = collision_1d_momentum(mass_a, velocity_a);
    double momentum_b FISICS_DIM(kg*m/s) = collision_1d_momentum(mass_b, velocity_b);
    double energy_a FISICS_DIM(energy) FISICS_UNIT(joule) = collision_1d_energy(mass_a, velocity_a);
    double energy_b FISICS_DIM(energy) FISICS_UNIT(joule) = collision_1d_energy(mass_b, velocity_b);

    printf("body velocity_mps momentum energy_j\n");
    printf("A %.3f %.3f %.3f\n", velocity_a, momentum_a, energy_a);
    printf("B %.3f %.3f %.3f\n", velocity_b, momentum_b, energy_b);
    return 0;
}
