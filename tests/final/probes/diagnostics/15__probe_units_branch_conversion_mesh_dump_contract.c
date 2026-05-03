static double select_force_newton(
    int metric_lane,
    double thrust_n [[fisics::dim(force)]] [[fisics::unit(newton)]],
    double thrust_lbf [[fisics::dim(force)]] [[fisics::unit(pound_force)]]) {
    if (metric_lane) return thrust_n;

    double converted
        [[fisics::dim(force)]]
        [[fisics::unit(newton)]] = fisics_convert_unit(thrust_lbf, "newton");
    return converted;
}

static double select_distance_meter(
    int metric_lane,
    double distance_m [[fisics::dim(length)]] [[fisics::unit(meter)]],
    double distance_ft [[fisics::dim(length)]] [[fisics::unit(foot)]]) {
    if (metric_lane) return distance_m;

    double converted
        [[fisics::dim(length)]]
        [[fisics::unit(meter)]] = fisics_convert_unit(distance_ft, "meter");
    return converted;
}

int main(void) {
    double total_force
        [[fisics::dim(force)]]
        [[fisics::unit(newton)]] = 0.0;
    double total_distance
        [[fisics::dim(length)]]
        [[fisics::unit(meter)]] = 0.0;

    total_force = total_force + select_force_newton(1, 25.0, 0.0);
    total_force = total_force + select_force_newton(0, 0.0, 8.0);
    total_distance = total_distance + select_distance_meter(1, 120.0, 0.0);
    total_distance = total_distance + select_distance_meter(0, 0.0, 42.0);

    return (int)total_force;
}
