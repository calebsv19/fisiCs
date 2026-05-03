int main(void) {
    double distance
        [[fisics::dim(length)]] = 3.048;

    return (int)fisics_convert_unit(distance, "foot");
}
