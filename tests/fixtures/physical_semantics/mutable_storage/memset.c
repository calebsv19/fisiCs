#include <fisics/extensions.h>
#define E FISICS_DIM(energy) FISICS_QUANTITY_KIND("quantity.energy")
#define Q FISICS_DIM(energy) FISICS_QUANTITY_KIND("quantity.torque")
void *memset(void*,int,unsigned long); E double check(double a E){double s[2];s[0]=a;memset(s,0,sizeof s);return s[0];}
