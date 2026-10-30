/* Source-shaped local 2D Euler-Bernoulli fixture, not a shared solver API.
 * Original order: ux_a, uy_a, rz_a, ux_b, uy_b, rz_b.
 * Translation block order: ux_a, uy_a, ux_b, uy_b; rotation: rz_a, rz_b.
 * Rotations have scalar dimensions (radians); richer quantity kinds are separate.
 * The numerical basis is SI. Product dimensions do not prove concrete-unit scale.
 * Preconditions: finite positive E, A, I and L, already admitted by the caller.
 * No releases, transforms, dense adapters, factorization or model admission here.
 */
#ifndef FISICS_TEST_FRAME_LOCAL_H
#define FISICS_TEST_FRAME_LOCAL_H
#include <fisics/extensions.h>

typedef struct FrameLocalSection {
    double young FISICS_DIM(pressure) FISICS_UNIT(pascal);
    double area FISICS_DIM(area) FISICS_UNIT(square_meter);
    double inertia FISICS_DIM(second_moment_of_area) FISICS_UNIT(meter_to_fourth);
    double length FISICS_DIM(length) FISICS_UNIT(meter);
} FrameLocalSection;

typedef struct FrameLocalStiffness {
    double tt[4][4] FISICS_DIM(force/length);
    double tr[4][2] FISICS_DIM(force);
    double rt[2][4] FISICS_DIM(force);
    double rr[2][2] FISICS_DIM(force*length);
} FrameLocalStiffness;

typedef struct FrameLocalCoordinates {
    double translation[4] FISICS_DIM(length) FISICS_UNIT(meter);
    double rotation[2] FISICS_DIM(scalar);
} FrameLocalCoordinates;

typedef struct FrameLocalLoads {
    double force[4] FISICS_DIM(force);
    double moment[2] FISICS_DIM(force*length);
} FrameLocalLoads;

static void frame_local_build(const FrameLocalSection *section, FrameLocalStiffness *k) {
    double axial FISICS_DIM(force/length) = section->young * section->area / section->length;
    double bending FISICS_DIM(force/length) = section->young * section->inertia /
        (section->length * section->length * section->length);
    double transverse FISICS_DIM(force/length) = 12 * bending;
    double coupling FISICS_DIM(force) = 6 * bending * section->length;
    double rotation_diagonal FISICS_DIM(force*length) = 4 * bending * section->length * section->length;
    double rotation_off_diagonal FISICS_DIM(force*length) = 2 * bending * section->length * section->length;
    double zero_tt FISICS_DIM(force/length) = 0;
    double zero_coupling FISICS_DIM(force) = 0;
    for (unsigned i = 0; i < 4; ++i) {
        for (unsigned j = 0; j < 4; ++j) k->tt[i][j] = zero_tt;
        for (unsigned j = 0; j < 2; ++j) k->tr[i][j] = zero_coupling;
    }
    for (unsigned i = 0; i < 2; ++i)
        for (unsigned j = 0; j < 4; ++j) k->rt[i][j] = zero_coupling;
    k->tt[0][0] = axial;
    k->tt[0][2] = -axial;
    k->tt[2][0] = -axial;
    k->tt[2][2] = axial;
    k->tt[1][1] = transverse;
    k->tt[1][3] = -transverse;
    k->tt[3][1] = -transverse;
    k->tt[3][3] = transverse;
    k->tr[1][0] = coupling;
    k->tr[1][1] = coupling;
    k->tr[3][0] = -coupling;
    k->tr[3][1] = -coupling;
    k->rt[0][1] = coupling;
    k->rt[0][3] = -coupling;
    k->rt[1][1] = coupling;
    k->rt[1][3] = -coupling;
    k->rr[0][0] = rotation_diagonal;
    k->rr[0][1] = rotation_off_diagonal;
    k->rr[1][0] = rotation_off_diagonal;
    k->rr[1][1] = rotation_diagonal;
}

static void frame_local_apply(const FrameLocalStiffness *k,
    const FrameLocalCoordinates *q, FrameLocalLoads *load) {
    double zero_force FISICS_DIM(force) = 0;
    double zero_moment FISICS_DIM(force*length) = 0;
    for (unsigned i = 0; i < 4; ++i) {
        load->force[i] = zero_force;
        for (unsigned j = 0; j < 4; ++j)
            load->force[i] += k->tt[i][j] * q->translation[j];
        for (unsigned j = 0; j < 2; ++j)
            load->force[i] += k->tr[i][j] * q->rotation[j];
    }
    for (unsigned i = 0; i < 2; ++i) {
        load->moment[i] = zero_moment;
        for (unsigned j = 0; j < 4; ++j)
            load->moment[i] += k->rt[i][j] * q->translation[j];
        for (unsigned j = 0; j < 2; ++j)
            load->moment[i] += k->rr[i][j] * q->rotation[j];
    }
}
#endif
