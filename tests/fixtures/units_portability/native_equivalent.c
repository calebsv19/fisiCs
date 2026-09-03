#include <stdio.h>

int main(void) {
    double distance [[fisics::dim(length)]] [[fisics::unit(meter)]] = 2.0;
    double speed [[fisics::dim(speed)]] [[fisics::unit(meter_per_second)]] = 3.0;
    printf("distance=%.3f speed=%.3f\n", distance, speed);
    return 0;
}
