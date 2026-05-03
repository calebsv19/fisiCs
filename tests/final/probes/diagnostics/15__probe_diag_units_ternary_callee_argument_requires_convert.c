typedef double distance_consumer_fn(
    double distance_m [[fisics::dim(length)]] [[fisics::unit(meter)]]);

static double consume_distance(
    double distance_m [[fisics::dim(length)]] [[fisics::unit(meter)]]) {
    return distance_m;
}

int main(void) {
    distance_consumer_fn* left_sink = consume_distance;
    distance_consumer_fn* right_sink = consume_distance;
    double distance_ft
        [[fisics::dim(length)]]
        [[fisics::unit(foot)]] = 21.0;
    int choose_left = 1;

    return (int)(choose_left ? left_sink : right_sink)(distance_ft);
}
