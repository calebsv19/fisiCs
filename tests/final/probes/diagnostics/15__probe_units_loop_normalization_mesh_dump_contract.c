static double normalize_leg(
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
    double route_meters
        [[fisics::dim(length)]]
        [[fisics::unit(meter)]] = 0.0;
    double total_energy
        [[fisics::dim(energy)]]
        [[fisics::unit(joule)]] = 0.0;
    double pack_kv
        [[fisics::dim(voltage)]]
        [[fisics::unit(kilovolt)]] = 0.0;

    for (int i = 0; i < 2; ++i) {
        double leg_miles
            [[fisics::dim(length)]]
            [[fisics::unit(mile)]] = 0.125 + (double)i * 0.125;
        double leg_meters
            [[fisics::dim(length)]]
            [[fisics::unit(meter)]] = normalize_leg(leg_miles);
        route_meters = route_meters + leg_meters;
    }

    for (int i = 0; i < 2; ++i) {
        double stage_wh
            [[fisics::dim(energy)]]
            [[fisics::unit(watt_hour)]] = 36.0 + (double)i * 12.0;
        double stage_j
            [[fisics::dim(energy)]]
            [[fisics::unit(joule)]] = normalize_stage_energy(stage_wh);
        total_energy = total_energy + stage_j;
    }

    for (int i = 0; i < 2; ++i) {
        double cell_mv
            [[fisics::dim(voltage)]]
            [[fisics::unit(millivolt)]] = 3480.0 + (double)i * 15.0;
        double cell_kv
            [[fisics::dim(voltage)]]
            [[fisics::unit(kilovolt)]] = normalize_cell_drop(cell_mv);
        pack_kv = pack_kv + cell_kv;
    }

    return (int)route_meters;
}
