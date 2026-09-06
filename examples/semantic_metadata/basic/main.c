#include <stdio.h>

#include <fisics/extensions.h>

FISICS_ID("signal.room.temperature")
FISICS_DOMAIN("thermal")
FISICS_ROLE("data.read")
double room_temperature = 294.0;

FISICS_ID("method.room.observe")
FISICS_DOMAIN("thermal")
FISICS_ROLE("sensor.read")
FISICS_PRODUCES("signal.room.temperature")
double observe_room_temperature(void) {
    return room_temperature;
}

FISICS_ID("method.room.control")
FISICS_DOMAIN("control")
FISICS_ROLE("control.decide")
FISICS_CONSUMES("signal.room.temperature")
FISICS_PRODUCES("device.room.fan.target")
int choose_fan_target(double temperature) {
    return temperature > 295.0 ? 1 : 0;
}

int main(void) {
    double temperature = observe_room_temperature();
    printf("temperature=%.1f fan_target=%d\n", temperature,
           choose_fan_target(temperature));
    return 0;
}
