#include <stdio.h>

#include <fisics/extensions.h>

FISICS_ID("signal.portable.temperature")
FISICS_DOMAIN("thermal")
FISICS_ROLE("data.read")
double portable_temperature FISICS_DIM(temperature) FISICS_UNIT(kelvin) = 294.0;

FISICS_ID("method.portable.read_temperature")
FISICS_DOMAIN("thermal")
FISICS_ROLE("sensor.read")
FISICS_PRODUCES("signal.portable.temperature")
double portable_read_temperature(void) {
    return portable_temperature;
}

int main(void) {
    printf("portable temperature=%.1f K\n", portable_read_temperature());
    return 0;
}
