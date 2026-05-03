int main(void) {
    int elapsed
        [[fisics::dim(time)]]
        [[fisics::unit(second)]] = 60;

    return fisics_convert_unit(elapsed, "minute");
}
