static double echo_distance(
    double distance_ft [[fisics::dim(length)]] [[fisics::unit(foot)]]) {
    return distance_ft;
}

static double consume_distance(
    double distance_m [[fisics::dim(length)]] [[fisics::unit(meter)]]) {
    return distance_m;
}

int main(void) {
    double distance_ft
        [[fisics::dim(length)]]
        [[fisics::unit(foot)]] = 18.0;

    return (int)consume_distance(echo_distance(distance_ft));
}
