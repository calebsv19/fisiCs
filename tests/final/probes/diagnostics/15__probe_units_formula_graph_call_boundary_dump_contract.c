static double normalize_speed(
    double launch_speed_fps [[fisics::dim(speed)]] [[fisics::unit(foot_per_second)]]) {
    double launch_speed_mps
        [[fisics::dim(speed)]]
        [[fisics::unit(meter_per_second)]] =
            fisics_convert_unit(launch_speed_fps, "meter_per_second");
    return launch_speed_mps;
}

static double stage_energy(
    double thrust_lbf [[fisics::dim(force)]] [[fisics::unit(pound_force)]],
    double burn_time [[fisics::dim(time)]] [[fisics::unit(second)]],
    double launch_speed_fps [[fisics::dim(speed)]] [[fisics::unit(foot_per_second)]]) {
    double thrust_n
        [[fisics::dim(force)]]
        [[fisics::unit(newton)]] = fisics_convert_unit(thrust_lbf, "newton");
    double speed_mps
        [[fisics::dim(speed)]]
        [[fisics::unit(meter_per_second)]] = normalize_speed(launch_speed_fps);
    double travel_distance
        [[fisics::dim(length)]]
        [[fisics::unit(meter)]] = speed_mps * burn_time;
    double work_j
        [[fisics::dim(energy)]]
        [[fisics::unit(joule)]] = thrust_n * travel_distance;
    return work_j;
}

int main(void) {
    double launch_speed_fps
        [[fisics::dim(speed)]]
        [[fisics::unit(foot_per_second)]] = 48.0;
    double burn_time
        [[fisics::dim(time)]]
        [[fisics::unit(second)]] = 6.0;
    double thrust_lbf
        [[fisics::dim(force)]]
        [[fisics::unit(pound_force)]] = 18.0;
    double work_j
        [[fisics::dim(energy)]]
        [[fisics::unit(joule)]] =
            stage_energy(thrust_lbf, burn_time, launch_speed_fps);
    return (int)work_j;
}
