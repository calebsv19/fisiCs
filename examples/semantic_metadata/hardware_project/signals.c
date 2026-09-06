#include <fisics/extensions.h>

FISICS_ID("signal.cabin.temperature")
FISICS_DOMAIN("thermal")
FISICS_ROLE("sensor.read")
double cabin_temperature FISICS_DIM(temperature) FISICS_UNIT(kelvin) = 294.0;

FISICS_ID("device.cabin.fan.command")
FISICS_DOMAIN("control")
FISICS_ROLE("data.read")
double fan_temperature_delta FISICS_DIM(temperature) FISICS_UNIT(kelvin) = 2.0;
