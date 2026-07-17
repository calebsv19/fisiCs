struct Sample {
    double distance
        [[fisics::dim(length)]]
        [[fisics::unit(meter)]];
};

int main(void) {
    double distance_ft
        [[fisics::dim(length)]]
        [[fisics::unit(foot)]] = 4.0;
    struct Sample sample = { .distance = distance_ft };
    return (int)sample.distance;
}
