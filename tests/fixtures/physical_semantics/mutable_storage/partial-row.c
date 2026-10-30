#include <fisics/extensions.h>
#define E FISICS_DIM(energy) FISICS_QUANTITY_KIND("quantity.energy")
#define Q FISICS_DIM(energy) FISICS_QUANTITY_KIND("quantity.torque")
E double check(double a E,double b Q){double s[2];double t[2][2];t[0][0]=a;double *p=t[0];return t[0][0];}
