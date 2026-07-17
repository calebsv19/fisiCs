int main(void) {
    double distance_ft
        [[fisics::dim(length)]]
        [[fisics::unit(foot)]] = 7.0;
    double distance_m
        [[fisics::dim(length)]]
        [[fisics::unit(meter)]] = 0.0;
    double *slot = &distance_m;
    *slot = distance_ft;
    return (int)distance_m;
}
