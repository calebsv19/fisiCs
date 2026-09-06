#include <fisics/extensions.h>

int main(void) {
    double mass FISICS_DIM(mass) FISICS_UNIT(kilogram) = 2.0;
    double velocity FISICS_DIM(speed) FISICS_UNIT(meter_per_second) = 3.0;
    double energy FISICS_DIM(energy) FISICS_UNIT(joule) = 0.0;

    energy = mass + velocity;
    return (int)energy;
}
