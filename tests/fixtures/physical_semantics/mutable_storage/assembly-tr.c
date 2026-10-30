#include <fisics/extensions.h>
#define E FISICS_DIM(energy) FISICS_QUANTITY_KIND("quantity.energy")
#define Q FISICS_DIM(energy) FISICS_QUANTITY_KIND("quantity.torque")
double FISICS_DIM(mass*length/time) FISICS_QUANTITY_KIND("quantity.momentum") check(double x FISICS_DIM(mass*length) FISICS_QUANTITY_KIND("quantity.inertia_translation_rotation"),double rate FISICS_DIM(1/time) FISICS_QUANTITY_KIND("quantity.angular_rate")){const unsigned map[3]={0,1,2};double matrix[36],mass[6][6];matrix[6*1+2]=x;mass[map[1]][map[2]]=x-x;mass[map[1]][map[2]]+=matrix[6*1+2];return rate*mass[map[1]][map[2]];}
