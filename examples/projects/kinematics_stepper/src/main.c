#include <stdio.h>

#include <fisics/extensions.h>

#include "kinematics_stepper.h"

int main(void) {
    double position FISICS_DIM(length) FISICS_UNIT(meter) = 0.0;
    double velocity FISICS_DIM(speed) FISICS_UNIT(meter_per_second) = 18.0;
    double acceleration FISICS_DIM(acceleration)
        FISICS_UNIT(meter_per_second_squared) = -9.81;
    double dt FISICS_DIM(time) FISICS_UNIT(second) = 0.1;
    double mass FISICS_DIM(mass) FISICS_UNIT(kilogram) = 1.25;
    double kinetic_energy FISICS_DIM(energy) FISICS_UNIT(joule) = 0.0;

    printf("step position_m velocity_mps kinetic_j\n");
    for (int step = 1; step <= 3; ++step) {
        KinematicsSample sample;

        velocity = velocity + acceleration * dt;
        position = position + velocity * dt;
        kinetic_energy = 0.5 * mass * velocity * velocity;

        kinematics_record(&sample, position, velocity, kinetic_energy);
        printf("%d %.3f %.3f %.3f\n",
               step,
               sample.position_m,
               sample.velocity_mps,
               sample.kinetic_j);
    }

    return 0;
}
