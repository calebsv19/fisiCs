[[fisics::id("signal.parity.temperature.input")]]
[[fisics::domain("thermal")]]
[[fisics::role("data.read")]]
double parity_temperature_input
    [[fisics::dim(temperature)]]
    [[fisics::unit(kelvin)]] = 294.0;

[[fisics::id("signal.parity.temperature.output")]]
[[fisics::domain("thermal")]]
[[fisics::role("data.write")]]
double parity_temperature_output
    [[fisics::dim(temperature)]]
    [[fisics::unit(kelvin)]] = 294.0;

[[fisics::id("method.parity.normalize")]]
[[fisics::domain("thermal")]]
[[fisics::role("data.write")]]
[[fisics::consumes("signal.parity.temperature.input")]]
[[fisics::produces("signal.parity.temperature.output")]]
double parity_normalize_temperature(double value) {
    return value;
}
