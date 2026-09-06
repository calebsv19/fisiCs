#include <fisics/extensions.h>

int main(void) {
    double velocity FISICS_DIM(speed) FISICS_UNIT(meter_per_second) = 18.0;
    double dt FISICS_DIM(time) FISICS_UNIT(second) = 0.1;
    double mass FISICS_DIM(mass) FISICS_UNIT(kilogram) = 1.25;
    double kinetic_energy FISICS_DIM(energy) FISICS_UNIT(joule) = 0.0;

    kinetic_energy = mass + velocity * dt;
    return (int)kinetic_energy;
}
