static double normalize_speed(
    double launch_speed_fps [[fisics::dim(speed)]] [[fisics::unit(foot_per_second)]]) {
    double launch_speed_mps
        [[fisics::dim(speed)]]
        [[fisics::unit(meter_per_second)]] =
            fisics_convert_unit(launch_speed_fps, "meter_per_second");
    return launch_speed_mps;
}

static double normalize_force(
    double thrust_lbf [[fisics::dim(force)]] [[fisics::unit(pound_force)]]) {
    double thrust_n
        [[fisics::dim(force)]]
        [[fisics::unit(newton)]] = fisics_convert_unit(thrust_lbf, "newton");
    return thrust_n;
}

static double normalize_reserve_energy(
    double reserve_wh [[fisics::dim(energy)]] [[fisics::unit(watt_hour)]]) {
    double reserve_j
        [[fisics::dim(energy)]]
        [[fisics::unit(joule)]] = fisics_convert_unit(reserve_wh, "joule");
    return reserve_j;
}

static double stage_distance(
    double launch_speed_fps [[fisics::dim(speed)]] [[fisics::unit(foot_per_second)]],
    double burn_time [[fisics::dim(time)]] [[fisics::unit(second)]]) {
    double launch_speed_mps
        [[fisics::dim(speed)]]
        [[fisics::unit(meter_per_second)]] = normalize_speed(launch_speed_fps);
    double travel_distance
        [[fisics::dim(length)]]
        [[fisics::unit(meter)]] = launch_speed_mps * burn_time;
    return travel_distance;
}

static double stage_total_energy(
    double reserve_wh [[fisics::dim(energy)]] [[fisics::unit(watt_hour)]],
    double thrust_lbf [[fisics::dim(force)]] [[fisics::unit(pound_force)]],
    double launch_speed_fps [[fisics::dim(speed)]] [[fisics::unit(foot_per_second)]],
    double burn_time [[fisics::dim(time)]] [[fisics::unit(second)]]) {
    double reserve_j
        [[fisics::dim(energy)]]
        [[fisics::unit(joule)]] = normalize_reserve_energy(reserve_wh);
    double thrust_n
        [[fisics::dim(force)]]
        [[fisics::unit(newton)]] = normalize_force(thrust_lbf);
    double travel_distance
        [[fisics::dim(length)]]
        [[fisics::unit(meter)]] = stage_distance(launch_speed_fps, burn_time);
    double work_j
        [[fisics::dim(energy)]]
        [[fisics::unit(joule)]] = thrust_n * travel_distance;
    double total_j
        [[fisics::dim(energy)]]
        [[fisics::unit(joule)]] = reserve_j + work_j;
    return total_j;
}

int main(void) {
    double stage_a_energy
        [[fisics::dim(energy)]]
        [[fisics::unit(joule)]] =
            stage_total_energy(24.0, 9.0, 44.0, 3.0);
    return (int)stage_a_energy;
}
