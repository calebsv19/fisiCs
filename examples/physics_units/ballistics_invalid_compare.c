int main(void) {
    double position [[fisics::dim(length)]] [[fisics::unit(meter)]] = 0.0;
    double dt [[fisics::dim(time)]] [[fisics::unit(second)]] = 0.1;

    return position < dt;
}
