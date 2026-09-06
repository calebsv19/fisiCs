[[fisics::id("method.cabin.control")]]
[[fisics::domain("control")]]
[[fisics::role("control.decide")]]
[[fisics::consumes("signal.cabin.temperature")]]
[[fisics::produces("device.cabin.fan.command")]]
int update_cabin_control(void) { return 0; }
