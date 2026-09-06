[[fisics::id("method.cabin.read_environment")]]
[[fisics::domain("thermal")]]
[[fisics::role("sensor.read")]]
[[fisics::produces("signal.cabin.temperature")]]
[[fisics::produces("signal.cabin.humidity")]]
double read_environment(void);

[[fisics::id("signal.cabin.temperature")]]
[[fisics::domain("thermal")]]
[[fisics::role("data.read")]]
double cabin_temperature = 21.0;

[[fisics::domain("control")]]
[[fisics::role("control.decide")]]
[[fisics::consumes("signal.cabin.temperature")]]
[[fisics::produces("device.roof_fan.target")]]
void control_without_id(void) {}

[[fisics::id("method.cabin.read_environment")]]
[[fisics::domain("thermal")]]
[[fisics::role("sensor.read")]]
[[fisics::produces("signal.cabin.temperature")]]
double read_environment(void) { return cabin_temperature; }
