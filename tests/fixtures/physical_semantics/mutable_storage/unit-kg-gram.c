#include <fisics/extensions.h>
#define E FISICS_DIM(energy) FISICS_QUANTITY_KIND("quantity.energy")
#define Q FISICS_DIM(energy) FISICS_QUANTITY_KIND("quantity.torque")
double FISICS_DIM(mass) FISICS_UNIT(kilogram) FISICS_QUANTITY_KIND("quantity.mass") check(double a FISICS_DIM(mass) FISICS_UNIT(gram) FISICS_QUANTITY_KIND("quantity.mass")){double s[1];s[0]=a;return s[0];}
