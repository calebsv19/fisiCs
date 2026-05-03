typedef double distance_consumer_fn(
    double distance_m [[fisics::dim(length)]] [[fisics::unit(meter)]]);

struct distance_pipeline {
    distance_consumer_fn* sink;
};

static double consume_distance(
    double distance_m [[fisics::dim(length)]] [[fisics::unit(meter)]]) {
    return distance_m;
}

int main(void) {
    struct distance_pipeline pipeline = {consume_distance};
    double distance_ft
        [[fisics::dim(length)]]
        [[fisics::unit(foot)]] = 24.0;

    return (int)pipeline.sink(distance_ft);
}
