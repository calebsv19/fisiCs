typedef double distance_consumer_fn(
    double distance_m [[fisics::dim(length)]] [[fisics::unit(meter)]]);

static double consume_distance(
    double distance_m [[fisics::dim(length)]] [[fisics::unit(meter)]]) {
    return distance_m;
}

static double forward_distance(
    distance_consumer_fn* sink,
    double distance_ft [[fisics::dim(length)]] [[fisics::unit(foot)]]) {
    return sink(distance_ft);
}

int main(void) {
    double distance_ft
        [[fisics::dim(length)]]
        [[fisics::unit(foot)]] = 9.0;

    return (int)forward_distance(consume_distance, distance_ft);
}
