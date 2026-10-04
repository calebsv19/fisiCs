#include <fisics/extensions.h>

FISICS_ID("controller.bench.can_supervisor")
FISICS_DOMAIN("control")
FISICS_ROLE("control.decide")
FISICS_CONSUMES("signal.bench.ultrasonic.distance")
FISICS_CONSUMES("signal.bench.environment.air_temperature")
FISICS_CONSUMES("signal.bench.environment.relative_humidity")
void observe_bench_can_signals(void) {}
