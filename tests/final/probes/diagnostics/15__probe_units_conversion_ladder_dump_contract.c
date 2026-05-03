static double normalize_leg_distance(
    double leg_miles [[fisics::dim(length)]] [[fisics::unit(mile)]]) {
    double leg_meters
        [[fisics::dim(length)]]
        [[fisics::unit(meter)]] = fisics_convert_unit(leg_miles, "meter");
    return leg_meters;
}

static double normalize_stage_energy(
    double stage_wh [[fisics::dim(energy)]] [[fisics::unit(watt_hour)]]) {
    double stage_j
        [[fisics::dim(energy)]]
        [[fisics::unit(joule)]] = fisics_convert_unit(stage_wh, "joule");
    return stage_j;
}

static double normalize_cell_drop(
    double cell_mv [[fisics::dim(voltage)]] [[fisics::unit(millivolt)]]) {
    double cell_kv
        [[fisics::dim(voltage)]]
        [[fisics::unit(kilovolt)]] = fisics_convert_unit(cell_mv, "kilovolt");
    return cell_kv;
}

int main(void) {
    double leg_a_miles
        [[fisics::dim(length)]]
        [[fisics::unit(mile)]] = 0.25;
    double leg_b_miles
        [[fisics::dim(length)]]
        [[fisics::unit(mile)]] = 0.125;
    double leg_a_meters
        [[fisics::dim(length)]]
        [[fisics::unit(meter)]] = normalize_leg_distance(leg_a_miles);
    double leg_b_meters
        [[fisics::dim(length)]]
        [[fisics::unit(meter)]] = normalize_leg_distance(leg_b_miles);
    double route_meters
        [[fisics::dim(length)]]
        [[fisics::unit(meter)]] = leg_a_meters + leg_b_meters;

    double stage_a_wh
        [[fisics::dim(energy)]]
        [[fisics::unit(watt_hour)]] = 80.0;
    double stage_b_wh
        [[fisics::dim(energy)]]
        [[fisics::unit(watt_hour)]] = 45.0;
    double stage_a_j
        [[fisics::dim(energy)]]
        [[fisics::unit(joule)]] = normalize_stage_energy(stage_a_wh);
    double stage_b_j
        [[fisics::dim(energy)]]
        [[fisics::unit(joule)]] = normalize_stage_energy(stage_b_wh);
    double total_energy
        [[fisics::dim(energy)]]
        [[fisics::unit(joule)]] = stage_a_j + stage_b_j;

    double cell_a_mv
        [[fisics::dim(voltage)]]
        [[fisics::unit(millivolt)]] = 3500.0;
    double cell_b_mv
        [[fisics::dim(voltage)]]
        [[fisics::unit(millivolt)]] = 3475.0;
    double cell_a_kv
        [[fisics::dim(voltage)]]
        [[fisics::unit(kilovolt)]] = normalize_cell_drop(cell_a_mv);
    double cell_b_kv
        [[fisics::dim(voltage)]]
        [[fisics::unit(kilovolt)]] = normalize_cell_drop(cell_b_mv);
    double pack_kv
        [[fisics::dim(voltage)]]
        [[fisics::unit(kilovolt)]] = cell_a_kv + cell_b_kv;

    return 0;
}
