#include <fisics/extensions.h>

int main(void) {
    double position FISICS_DIM(length) FISICS_UNIT(meter) = 0.0;
    double dt FISICS_DIM(time) FISICS_UNIT(second) = 0.1;

    return position < dt;
}
