[[fisics::id("method.cabin.control")]]
[[fisics::domain("control")]]
[[fisics::role("control.decide")]]
[[fisics::consumes("signal.cabin.temperature")]]
[[fisics::consumes("signal.cabin.temperature_delta")]]
[[fisics::consumes("signal.cabin.velocity")]]
[[fisics::consumes("quantity.cabin.energy")]]
[[fisics::consumes("quantity.cabin.torque")]]
[[fisics::consumes("signal.cabin.uninstrumented")]]
[[fisics::produces("device.cabin.fan.command")]]
void update_cabin_control(void) {}

[[fisics::id("method.cabin.self_observe")]]
[[fisics::domain("control")]]
[[fisics::role("control.decide")]]
[[fisics::consumes("method.cabin.self_observe")]]
void self_observe(void) {}
