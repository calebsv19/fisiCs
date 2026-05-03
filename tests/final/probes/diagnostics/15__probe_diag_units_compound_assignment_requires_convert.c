int main(void) {
    double reserve_charge
        [[fisics::dim(charge)]]
        [[fisics::unit(ampere_hour)]] = 1.0;
    double recharge_step
        [[fisics::dim(charge)]]
        [[fisics::unit(milliampere_hour)]] = 250.0;

    reserve_charge += recharge_step;
    return (int)reserve_charge;
}
