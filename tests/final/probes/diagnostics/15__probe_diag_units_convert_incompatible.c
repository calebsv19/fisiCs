int main(void) {
    double distance_m
        [[fisics::dim(length)]]
        [[fisics::unit(meter)]] = 3.048;

    return (int)fisics_convert_unit(distance_m, "second");
}
