#include <stdio.h>

int main(void) {
    double position [[fisics::dim(length)]] [[fisics::unit(meter)]] = 0.0;
    double velocity [[fisics::dim(speed)]] [[fisics::unit(meter_per_second)]] = 18.0;
    double acceleration [[fisics::dim(acceleration)]] [[fisics::unit(meter_per_second_squared)]] = 0.0;
    double dt [[fisics::dim(time)]] [[fisics::unit(second)]] = 0.1;
    double gravity [[fisics::dim(acceleration)]] [[fisics::unit(meter_per_second_squared)]] = -9.81;
    double mass [[fisics::dim(mass)]] [[fisics::unit(kilogram)]] = 1.25;
    double kinetic_energy [[fisics::dim(energy)]] [[fisics::unit(joule)]] = 0.0;

    acceleration = gravity;
    velocity = velocity + acceleration * dt;
    position = position + velocity * dt;
    kinetic_energy = 0.5 * mass * velocity * velocity;

    printf("position=%.3f velocity=%.3f energy=%.3f\n", position, velocity, kinetic_energy);
    return 0;
}
