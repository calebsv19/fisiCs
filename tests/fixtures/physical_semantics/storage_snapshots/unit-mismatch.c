#include <fisics/extensions.h>
#define E FISICS_DIM(energy) FISICS_QUANTITY_KIND("quantity.energy")
#define Q FISICS_DIM(energy) FISICS_QUANTITY_KIND("quantity.torque")
#define V FISICS_DIM(length/time) FISICS_QUANTITY_KIND("quantity.velocity")
#define W FISICS_DIM(1/time) FISICS_QUANTITY_KIND("quantity.angular_rate")
#define M FISICS_DIM(mass) FISICS_QUANTITY_KIND("quantity.mass")
M double check(double a M FISICS_UNIT(gram)) FISICS_UNIT(kilogram){const double s[1]={a};return s[0];}
