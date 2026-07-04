#include <stdio.h>

#include "kinematics_stepper.h"

int main(void) {
    double position [[fisics::dim(length)]] [[fisics::unit(meter)]] = 0.0;
    double velocity [[fisics::dim(speed)]] [[fisics::unit(meter_per_second)]] = 18.0;
    double acceleration [[fisics::dim(acceleration)]] [[fisics::unit(meter_per_second_squared)]] = -9.81;
    double dt [[fisics::dim(time)]] [[fisics::unit(second)]] = 0.1;
    double mass [[fisics::dim(mass)]] [[fisics::unit(kilogram)]] = 1.25;
    double kinetic_energy [[fisics::dim(energy)]] [[fisics::unit(joule)]] = 0.0;

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
