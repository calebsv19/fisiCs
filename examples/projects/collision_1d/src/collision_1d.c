#include "collision_1d.h"

void collision_1d_elastic(CollisionBody1D* a, CollisionBody1D* b) {
    double m1 = a->mass_kg;
    double m2 = b->mass_kg;
    double u1 = a->velocity_mps;
    double u2 = b->velocity_mps;
    double sum = m1 + m2;

    a->velocity_mps = ((m1 - m2) / sum) * u1 + ((2.0 * m2) / sum) * u2;
    b->velocity_mps = ((2.0 * m1) / sum) * u1 + ((m2 - m1) / sum) * u2;
}

double collision_1d_momentum(double mass_kg, double velocity_mps) {
    return mass_kg * velocity_mps;
}

double collision_1d_energy(double mass_kg, double velocity_mps) {
    return 0.5 * mass_kg * velocity_mps * velocity_mps;
}
