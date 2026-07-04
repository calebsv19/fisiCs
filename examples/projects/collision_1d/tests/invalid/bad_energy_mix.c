int main(void) {
    double mass [[fisics::dim(mass)]] [[fisics::unit(kilogram)]] = 2.0;
    double velocity [[fisics::dim(speed)]] [[fisics::unit(meter_per_second)]] = 3.0;
    double energy [[fisics::dim(energy)]] [[fisics::unit(joule)]] = 0.0;

    energy = mass + velocity;
    return (int)energy;
}
