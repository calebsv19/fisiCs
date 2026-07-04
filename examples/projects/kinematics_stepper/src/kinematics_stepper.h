#ifndef KINEMATICS_STEPPER_H
#define KINEMATICS_STEPPER_H

typedef struct KinematicsSample {
    double position_m;
    double velocity_mps;
    double kinetic_j;
} KinematicsSample;

void kinematics_record(KinematicsSample* sample,
                       double position_m,
                       double velocity_mps,
                       double kinetic_j);

#endif
