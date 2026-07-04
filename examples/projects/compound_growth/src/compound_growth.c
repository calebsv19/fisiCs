#include "compound_growth.h"

double compound_growth_next(double balance, const GrowthConfig* config) {
    double interest = balance * config->rate_per_tick;
    return balance + interest + config->periodic_contribution;
}
