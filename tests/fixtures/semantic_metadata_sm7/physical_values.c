[[fisics::id("signal.cabin.temperature")]]
[[fisics::domain("thermal")]]
[[fisics::role("sensor.read")]]
[[fisics::dim(temperature)]]
[[fisics::unit(kelvin)]]
double cabin_temperature = 294.0;

[[fisics::id("signal.cabin.temperature_delta")]]
[[fisics::domain("thermal")]]
[[fisics::role("data.read")]]
[[fisics::dim(temperature)]]
[[fisics::unit(kelvin)]]
double cabin_temperature_delta = 2.0;

[[fisics::id("signal.cabin.velocity")]]
[[fisics::domain("physics")]]
[[fisics::role("data.read")]]
[[fisics::dim(velocity)]]
[[fisics::unit(foot_per_second)]]
double cabin_velocity = 3.0;

[[fisics::id("quantity.cabin.energy")]]
[[fisics::domain("physics")]]
[[fisics::role("data.read")]]
[[fisics::dim(energy)]]
[[fisics::unit(joule)]]
double stored_energy = 10.0;

[[fisics::id("quantity.cabin.torque")]]
[[fisics::domain("physics")]]
[[fisics::role("data.read")]]
[[fisics::dim(energy)]]
double shaft_torque = 5.0;

[[fisics::domain("physics")]]
[[fisics::role("data.read")]]
[[fisics::dim(length)]]
[[fisics::unit(foot)]]
double fallback_distance = 4.0;

[[fisics::id("signal.cabin.uninstrumented")]]
[[fisics::domain("physics")]]
[[fisics::role("data.read")]]
double uninstrumented_value = 0.0;

void retain_unbound_units_evidence(void) {
    double internal_duration [[fisics::dim(time)]] [[fisics::unit(second)]] = 1.0;
    (void)internal_duration;
}
