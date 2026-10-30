#include "contact_energy.h"
#include <math.h>
int main(void){
 double e9 CE_E=9,e5 CE_E=5,e0 CE_E=0,e57 CE_E=57.6,et CE_E=1e-13;
 double p4 CE_P=4,pt CE_P=1e-14;
 if(fabs(ce_kinetic(2,3)-e9)>et)return 1;
 if(fabs(ce_momentum_delta(2,3,1)-p4)>pt)return 2;
 if(fabs(ce_impulse(3,1,2)-p4)>pt)return 3;
 if(fabs(ce_loss(9,4)-e5)>et)return 4;
 if(fabs(ce_balance(4,9,5)-e0)>et)return 5;
 if(fabs(ce_spin(2,3,4)-e57)>et)return 6;
 return 0;
}
