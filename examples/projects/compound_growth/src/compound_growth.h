#ifndef COMPOUND_GROWTH_H
#define COMPOUND_GROWTH_H

typedef struct GrowthConfig {
    double starting_balance;
    double periodic_contribution;
    double rate_per_tick;
    int ticks;
} GrowthConfig;

double compound_growth_next(double balance, const GrowthConfig* config);

#endif
