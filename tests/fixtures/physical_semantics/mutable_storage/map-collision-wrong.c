#include <fisics/extensions.h>
#define E FISICS_DIM(energy) FISICS_QUANTITY_KIND("quantity.energy")
#define Q FISICS_DIM(energy) FISICS_QUANTITY_KIND("quantity.torque")
E double check(double a E,double b Q){const unsigned map[2]={0,0};double s[2];s[map[0]]=a;s[map[1]]+=b;return s[0];}
