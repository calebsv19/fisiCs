int main(void) {
    double climb_rate
        [[fisics::dim(speed)]]
        [[fisics::unit(meter_per_second)]] = 2.0;
    double descent_rate
        [[fisics::dim(speed)]]
        [[fisics::unit(foot_per_second)]] = 6.5;
    int prefer_descent = 1;

    double selected_rate
        [[fisics::dim(speed)]]
        [[fisics::unit(meter_per_second)]] =
            prefer_descent ? climb_rate : descent_rate;

    return (int)selected_rate;
}
