#include <fisics/extensions.h>

FISICS_ID("signal.parity.temperature.input")
FISICS_DOMAIN("thermal")
FISICS_ROLE("data.read")
double parity_temperature_input
    FISICS_DIM(temperature)
    FISICS_UNIT(kelvin) = 294.0;

FISICS_ID("signal.parity.temperature.output")
FISICS_DOMAIN("thermal")
FISICS_ROLE("data.write")
double parity_temperature_output
    FISICS_DIM(temperature)
    FISICS_UNIT(kelvin) = 294.0;

FISICS_ID("method.parity.normalize")
FISICS_DOMAIN("thermal")
FISICS_ROLE("data.write")
FISICS_CONSUMES("signal.parity.temperature.input")
FISICS_PRODUCES("signal.parity.temperature.output")
double parity_normalize_temperature(double value) {
    return value;
}
