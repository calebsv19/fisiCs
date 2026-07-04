#ifndef POPULATION_COMPETITION_H
#define POPULATION_COMPETITION_H

typedef struct PopulationState {
    int alpha;
    int beta;
    int resource;
} PopulationState;

typedef struct PopulationRule {
    int alpha_growth;
    int beta_growth;
    int resource_cost;
    int conflict_loss;
} PopulationRule;

void population_step(PopulationState* state, const PopulationRule* rule, int tick);
int population_state_valid(const PopulationState* state);

#endif
