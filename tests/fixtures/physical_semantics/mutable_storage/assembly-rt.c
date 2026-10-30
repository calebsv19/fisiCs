#include <fisics/extensions.h>
#define E FISICS_DIM(energy) FISICS_QUANTITY_KIND("quantity.energy")
#define Q FISICS_DIM(energy) FISICS_QUANTITY_KIND("quantity.torque")
double FISICS_DIM(mass*length*length/time) FISICS_QUANTITY_KIND("quantity.rotational_momentum") check(double x FISICS_DIM(mass*length) FISICS_QUANTITY_KIND("quantity.inertia_rotation_translation"),double rate FISICS_DIM(length/time) FISICS_QUANTITY_KIND("quantity.velocity")){const unsigned map[3]={0,1,2};double matrix[36],mass[6][6];matrix[6*2+1]=x;mass[map[2]][map[1]]=x-x;mass[map[2]][map[1]]+=matrix[6*2+1];return rate*mass[map[2]][map[1]];}
