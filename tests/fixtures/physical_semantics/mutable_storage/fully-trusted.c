#include <fisics/extensions.h>
#define E FISICS_DIM(energy) FISICS_QUANTITY_KIND("quantity.energy")
#define Q FISICS_DIM(energy) FISICS_QUANTITY_KIND("quantity.torque")
E double external(void); E double check(void){double s[2];s[0]=external();return s[0];}
