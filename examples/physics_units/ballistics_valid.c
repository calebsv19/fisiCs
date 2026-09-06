#include <stdio.h>

#include <fisics/extensions.h>

int main(void) {
    double position FISICS_DIM(length) FISICS_UNIT(meter) = 0.0;
    double velocity FISICS_DIM(speed) FISICS_UNIT(meter_per_second) = 18.0;
    double acceleration FISICS_DIM(acceleration)
        FISICS_UNIT(meter_per_second_squared) = 0.0;
    double dt FISICS_DIM(time) FISICS_UNIT(second) = 0.1;
    double gravity FISICS_DIM(acceleration)
        FISICS_UNIT(meter_per_second_squared) = -9.81;
    double mass FISICS_DIM(mass) FISICS_UNIT(kilogram) = 1.25;
    double kinetic_energy FISICS_DIM(energy) FISICS_UNIT(joule) = 0.0;

    acceleration = gravity;
    velocity = velocity + acceleration * dt;
    position = position + velocity * dt;
    kinetic_energy = 0.5 * mass * velocity * velocity;

    printf("position=%.3f velocity=%.3f energy=%.3f\n", position, velocity, kinetic_energy);
    return 0;
}
