#ifndef COLLISION_1D_H
#define COLLISION_1D_H

typedef struct CollisionBody1D {
    double mass_kg;
    double velocity_mps;
} CollisionBody1D;

void collision_1d_elastic(CollisionBody1D* a, CollisionBody1D* b);
double collision_1d_momentum(double mass_kg, double velocity_mps);
double collision_1d_energy(double mass_kg, double velocity_mps);

#endif
