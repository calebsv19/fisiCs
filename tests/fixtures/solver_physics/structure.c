/* Reduced from core_structural_frame_element and frame_recovery.
 * The 6x6 stiffness matrix has heterogeneous translational/rotational entries;
 * it cannot be assigned one homogeneous physical dimension. */
#include <fisics/extensions.h>
#include <stdio.h>
int main(void) {
    double young FISICS_DIM(pressure) = 12000000;
    double area FISICS_DIM(area) = 0.02;
    double inertia FISICS_DIM(second_moment_of_area) = 0.0004;
    double length FISICS_DIM(length) = 2;
    double axial FISICS_DIM(force/length) = young*area/length;
    double bend FISICS_DIM(force/length) = young*inertia/(length*length*length);
    double local[6][6] = {{0}};
    local[0][0] = axial;
    local[1][1] = 12*bend;
    local[1][2] = 6*bend*length;
    local[2][2] = 4*bend*length*length;
#ifdef BAD_MATRIX_SLOT
    local[0][0] = local[2][2]; /* desired: reject moment/rotation as force/length */
#endif
    double displacement FISICS_DIM(length) = 0.001;
    double force FISICS_DIM(force) = local[0][0]*displacement;
    double modulus FISICS_DIM(length*length*length) = 0.002;
    double moment FISICS_DIM(force*length) = 200;
#ifdef BAD_STRESS
    double stress FISICS_DIM(pressure) = force/area+moment/area;
#else
    double stress FISICS_DIM(pressure) = force/area+moment/modulus;
#endif
    printf("%.9f %.9f %.9f\n",local[0][0],force,stress);
    return 0;
}
