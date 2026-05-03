typedef double distance_consumer_fn(
    double distance_m [[fisics::dim(length)]] [[fisics::unit(meter)]]);

static double consume_distance(
    double distance_m [[fisics::dim(length)]] [[fisics::unit(meter)]]) {
    return distance_m;
}

static distance_consumer_fn* choose_sink(void) {
    return consume_distance;
}

int main(void) {
    double distance_ft
        [[fisics::dim(length)]]
        [[fisics::unit(foot)]] = 16.0;

    return (int)choose_sink()(distance_ft);
}
