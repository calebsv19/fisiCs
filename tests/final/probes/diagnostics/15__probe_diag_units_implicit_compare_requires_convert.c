int main(void) {
    double altitude
        [[fisics::dim(length)]]
        [[fisics::unit(meter)]] = 120.0;
    double ceiling
        [[fisics::dim(length)]]
        [[fisics::unit(foot)]] = 300.0;

    return (altitude < ceiling) ? 1 : 0;
}
