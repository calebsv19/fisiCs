struct Sample {
    double distance
        [[fisics::dim(length)]]
        [[fisics::unit(meter)]];
};

int main(void) {
    double distance_ft
        [[fisics::dim(length)]]
        [[fisics::unit(foot)]] = 5.0;
    struct Sample sample = { 0.0 };
    sample.distance = distance_ft;
    return (int)sample.distance;
}
