#include <fisics/extensions.h>

FISICS_ID("method.cabin.control")
FISICS_DOMAIN("control")
FISICS_ROLE("control.decide")
FISICS_CONSUMES("signal.cabin.temperature")
FISICS_PRODUCES("device.cabin.fan.command")
void update_cabin_control(void) {}
