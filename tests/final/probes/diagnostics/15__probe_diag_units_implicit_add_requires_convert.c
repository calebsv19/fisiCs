int main(void) {
    double distance_m
        [[fisics::dim(length)]]
        [[fisics::unit(meter)]] = 3.048;
    double distance_ft
        [[fisics::dim(length)]]
        [[fisics::unit(foot)]] = 10.0;

    return (int)(distance_m + distance_ft);
}
