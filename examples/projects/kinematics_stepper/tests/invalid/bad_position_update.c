#include <fisics/extensions.h>

int main(void) {
    double position FISICS_DIM(length) FISICS_UNIT(meter) = 0.0;
    double acceleration FISICS_DIM(acceleration)
        FISICS_UNIT(meter_per_second_squared) = -9.81;

    position = position + acceleration;
    return (int)position;
}
