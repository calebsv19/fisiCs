#include <stdio.h>

#include "compound_growth.h"

int main(void) {
    GrowthConfig config = {
        .starting_balance = 1000.0,
        .periodic_contribution = 50.0,
        .rate_per_tick = 0.01,
        .ticks = 4
    };

    double balance = config.starting_balance;
    printf("tick balance\n");
    printf("0 %.2f\n", balance);
    for (int tick = 1; tick <= config.ticks; ++tick) {
        balance = compound_growth_next(balance, &config);
        printf("%d %.2f\n", tick, balance);
    }

    return 0;
}
