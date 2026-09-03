#include <stdio.h>

#include <fisics/extensions.h>

int main(void) {
    double distance FISICS_DIM(length) FISICS_UNIT(meter) = 2.0;
    double speed FISICS_DIM(speed) FISICS_UNIT(meter_per_second) = 3.0;
    printf("distance=%.3f speed=%.3f\n", distance, speed);
    return 0;
}
