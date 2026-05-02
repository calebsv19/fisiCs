int main(void) {
    double velocity [[fisics::dim(speed)]] [[fisics::unit(meter_per_second)]] = 18.0;
    double dt [[fisics::dim(time)]] [[fisics::unit(second)]] = 0.1;
    double mass [[fisics::dim(mass)]] [[fisics::unit(kilogram)]] = 1.25;
    double kinetic_energy [[fisics::dim(energy)]] [[fisics::unit(joule)]] = 0.0;

    kinetic_energy = mass + velocity * dt;
    return (int)kinetic_energy;
}
