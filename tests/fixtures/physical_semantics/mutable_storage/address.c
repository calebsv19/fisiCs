#include <fisics/extensions.h>
#define E FISICS_DIM(energy) FISICS_QUANTITY_KIND("quantity.energy")
#define Q FISICS_DIM(energy) FISICS_QUANTITY_KIND("quantity.torque")
E double check(double a E,double b Q){double s[2];s[0]=a;double *p=&s[0];*p=b;return s[0];}
