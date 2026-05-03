int main(void) {
    double distance_ft
        [[fisics::dim(length)]]
        [[fisics::unit(foot)]] = 12.0;
    double distance_m
        [[fisics::dim(length)]]
        [[fisics::unit(meter)]] = distance_ft;

    return (int)distance_m;
}
