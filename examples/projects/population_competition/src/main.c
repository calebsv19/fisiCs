#include <stdio.h>

#include "population_competition.h"

int main(void) {
    PopulationState state = {
        .alpha = 40,
        .beta = 28,
        .resource = 120
    };
    PopulationRule rule = {
        .alpha_growth = 4,
        .beta_growth = 2,
        .resource_cost = 4,
        .conflict_loss = 1
    };

    printf("tick alpha beta resource\n");
    printf("0 %d %d %d\n", state.alpha, state.beta, state.resource);
    for (int tick = 1; tick <= 4; ++tick) {
        population_step(&state, &rule, tick);
        if (!population_state_valid(&state)) {
            return 3;
        }
        printf("%d %d %d %d\n", tick, state.alpha, state.beta, state.resource);
    }

    return 0;
}
