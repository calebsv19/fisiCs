int main(void) {
    double position [[fisics::dim(length)]] [[fisics::unit(meter)]] = 0.0;
    double acceleration [[fisics::dim(acceleration)]] [[fisics::unit(meter_per_second_squared)]] = -9.81;

    position += acceleration;
    return (int)position;
}
