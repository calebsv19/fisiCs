[[fisics::id("signal.cabin.temperature")]]
[[fisics::domain("thermal")]]
[[fisics::role("sensor.read")]]
int cabin_temperature = 21;

[[fisics::domain("data_io")]]
[[fisics::role("telemetry.publish")]]
[[fisics::consumes("signal.cabin.temperature")]]
void publish_temperature(void) {}
