typedef double distance_consumer_fn(
    double distance_m [[fisics::dim(length)]] [[fisics::unit(meter)]]);

static double consume_distance(
    double distance_m [[fisics::dim(length)]] [[fisics::unit(meter)]]) {
    return distance_m;
}

static double relay_distance(
    distance_consumer_fn** table,
    int slot,
    double distance_ft [[fisics::dim(length)]] [[fisics::unit(foot)]]) {
    return table[slot](distance_ft);
}

int main(void) {
    distance_consumer_fn* table[1] = {consume_distance};
    double distance_ft
        [[fisics::dim(length)]]
        [[fisics::unit(foot)]] = 13.0;

    return (int)relay_distance(table, 0, distance_ft);
}
