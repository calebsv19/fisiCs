int main(void) {
    double distance_ft
        [[fisics::dim(length)]]
        [[fisics::unit(foot)]] = 8.0;
    double primary_m
        [[fisics::dim(length)]]
        [[fisics::unit(meter)]] = 0.0;
    double backup_m
        [[fisics::dim(length)]]
        [[fisics::unit(meter)]] = 0.0;
    int use_primary = 1;
    *(use_primary ? &primary_m : &backup_m) = distance_ft;
    return (int)(primary_m + backup_m);
}
