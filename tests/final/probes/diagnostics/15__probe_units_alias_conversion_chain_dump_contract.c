double launch_speed
    [[fisics::dim(speed)]]
    [[fisics::unit(feet_per_second)]] = 60.0;
double launch_speed_mps
    [[fisics::dim(speed)]]
    [[fisics::unit(meter_per_second)]] = 0.0;

double load_kw
    [[fisics::dim(power)]]
    [[fisics::unit(kilowatts)]] = 1.5;
double load_w
    [[fisics::dim(power)]]
    [[fisics::unit(watt)]] = 0.0;

double cell_drop
    [[fisics::dim(voltage)]]
    [[fisics::unit(millivolts)]] = 250.0;
double pack_drop
    [[fisics::dim(voltage)]]
    [[fisics::unit(kilovolt)]] = 0.0;

int main(void) {
    launch_speed_mps = fisics_convert_unit(launch_speed, "meter_per_second");
    load_w = fisics_convert_unit(load_kw, "watt");
    pack_drop = fisics_convert_unit(cell_drop, "kilovolt");
    return 0;
}
