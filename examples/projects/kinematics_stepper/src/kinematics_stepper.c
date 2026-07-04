#include "kinematics_stepper.h"

void kinematics_record(KinematicsSample* sample,
                       double position_m,
                       double velocity_mps,
                       double kinetic_j) {
    sample->position_m = position_m;
    sample->velocity_mps = velocity_mps;
    sample->kinetic_j = kinetic_j;
}
