#include <fisics/extensions.h>

FISICS_ID("signal.bench.ultrasonic.distance")
FISICS_DOMAIN("hardware")
FISICS_ROLE("sensor.read")
double ultrasonic_distance
    FISICS_DIM(length)
    FISICS_UNIT(meter) = 0.5;

FISICS_ID("signal.bench.environment.air_temperature")
FISICS_DOMAIN("thermal")
FISICS_ROLE("sensor.read")
double environment_air_temperature
    FISICS_DIM(temperature)
    FISICS_UNIT(kelvin) = 294.0;

FISICS_ID("signal.bench.environment.relative_humidity")
FISICS_DOMAIN("thermal")
FISICS_ROLE("sensor.read")
double environment_relative_humidity
    FISICS_DIM(dimensionless) = 0.5;

FISICS_ID("provider.bench.ultrasonic")
FISICS_DOMAIN("hardware")
FISICS_ROLE("telemetry.publish")
FISICS_PRODUCES("signal.bench.ultrasonic.distance")
void publish_ultrasonic_distance(void) {}

FISICS_ID("provider.bench.environment")
FISICS_DOMAIN("thermal")
FISICS_ROLE("telemetry.publish")
FISICS_PRODUCES("signal.bench.environment.air_temperature")
FISICS_PRODUCES("signal.bench.environment.relative_humidity")
void publish_environment(void) {}
