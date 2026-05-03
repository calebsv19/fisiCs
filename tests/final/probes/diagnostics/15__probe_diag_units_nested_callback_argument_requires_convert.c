typedef double distance_consumer_fn(
    double distance_m [[fisics::dim(length)]] [[fisics::unit(meter)]]);

static double consume_distance(
    double distance_m [[fisics::dim(length)]] [[fisics::unit(meter)]]) {
    return distance_m;
}

static double forward_inner(
    distance_consumer_fn* sink,
    double distance_ft [[fisics::dim(length)]] [[fisics::unit(foot)]]) {
    return sink(distance_ft);
}

static double forward_outer(
    distance_consumer_fn* sink,
    double distance_ft [[fisics::dim(length)]] [[fisics::unit(foot)]]) {
    return forward_inner(sink, distance_ft);
}

int main(void) {
    double distance_ft
        [[fisics::dim(length)]]
        [[fisics::unit(foot)]] = 11.0;

    return (int)forward_outer(consume_distance, distance_ft);
}
