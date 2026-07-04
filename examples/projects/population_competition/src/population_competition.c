#include "population_competition.h"

static int clamp_nonnegative(int value) {
    return value < 0 ? 0 : value;
}

void population_step(PopulationState* state, const PopulationRule* rule, int tick) {
    int pressure = tick / 2;
    int alpha_gain = rule->alpha_growth - pressure;
    int beta_gain = rule->beta_growth - (tick / 3);
    int conflict = (state->alpha > state->beta) ? rule->conflict_loss : 0;

    state->alpha = clamp_nonnegative(state->alpha + alpha_gain - conflict);
    state->beta = clamp_nonnegative(state->beta + beta_gain);
    state->resource = clamp_nonnegative(state->resource - rule->resource_cost - tick);
}

int population_state_valid(const PopulationState* state) {
    return state->alpha >= 0 && state->beta >= 0 && state->resource >= 0;
}
