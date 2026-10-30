#include <fisics/extensions.h>
#define E FISICS_DIM(energy) FISICS_QUANTITY_KIND("quantity.energy")
#define Q FISICS_DIM(energy) FISICS_QUANTITY_KIND("quantity.torque")
E double check(double a E,double b Q){unsigned map[2]={0,1};double s[2];s[map[0]]=a;map[0]=1;return s[map[0]];}
