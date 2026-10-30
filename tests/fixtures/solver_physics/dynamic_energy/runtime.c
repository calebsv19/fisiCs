#include "core_structural_dynamic_energy.h"
#include <math.h>
#include <stdio.h>
static int near(double x,double y){return fabs(x-y)<1e-12;}
int main(void){
 if(!near(cs_de_kinetic_tt(2,3,4),12)||!near(cs_de_kinetic_rr(2,5,4),20)||!near(cs_de_kinetic_tr(2,6,3),18)||!near(cs_de_kinetic_rt(2,6,3),18))return 1;
 if(!near(cs_de_damping_tt(.1,2,1,3,4,2,4),4.8)||!near(cs_de_damping_rr(.1,2,1,3,5,2,4),6))return 2;
 if(!near(cs_de_translation_work(2,4,1,5),12)||!near(cs_de_rotation_work(2,4,1,5),12)||!near(cs_de_balance(5,2,3,4,6),4))return 3;
 puts("dynamic energy arithmetic: 9 independent values PASS");return 0;
}
