int main(void) {
    double distance_ft
        [[fisics::dim(length)]]
        [[fisics::unit(foot)]] = 6.0;
    double distance_m[2]
        [[fisics::dim(length)]]
        [[fisics::unit(meter)]] = { 0.0, 0.0 };
    distance_m[1] = distance_ft;
    return (int)distance_m[1];
}
