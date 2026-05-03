typedef double distance_consumer_fn(
    double distance_m [[fisics::dim(length)]] [[fisics::unit(meter)]]);

static double consume_distance(
    double distance_m [[fisics::dim(length)]] [[fisics::unit(meter)]]) {
    return distance_m;
}

int main(void) {
    distance_consumer_fn* sink = consume_distance;
    double distance_ft
        [[fisics::dim(length)]]
        [[fisics::unit(foot)]] = 14.0;

    return (int)(0, sink)(distance_ft);
}
